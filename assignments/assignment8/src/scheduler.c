//!
//! @file scheduler.c
//! @brief Implementation of scheduler as a state machine to handle events triggered by interrupts
//! @version 0.1
//!
//! @date 2020-09-24
//! @author Roberto Baquerizo (roba8460@colorado.edu)
//!
//! @institution University of Colorado Boulder (UCB)
//! @course ECEN 5823-001: IoT Embedded Firmware (Fall 2020)
//! @instructor David Sluiter
//!
//! @assignment ecen5823-assignment7-baquerrj
//!
//! @resources Utilized Silicon Labs' EMLIB peripheral libraries to implement functionality
//!
//! @copyright All rights reserved. Distribution allowed only for the use of assignment grading. Use of code excerpts allowed at the discretion of author. Contact for permission.
//!

#include "scheduler.h"

#include "log.h"
#include "gpio.h"
#include "i2c.h"
#include "timers.h"
#include "ble.h"
#include "display.h"

#include "gecko_ble_errors.h"
#include "gatt_db.h"
#include "infrastructure.h"

//! Stores the current state of the scheduler's state machine
static schedulerStates_e currentState = STATE_SENSOR_OFF;

//! Stores the state the state machine will transition to
static schedulerStates_e nextState = STATE_WAIT_FOR_POWERUP;

//! Constant defining the starting state for the state machine
//! Used when resetting the state machine when a connection is closed
static const schedulerEvents_e startState = STATE_SENSOR_OFF;

//! shutdown()
//! @brief Convenience function to disable Si7021, I2C and @n
//! any sleep blocks. Forces transition to @ref startState
//!
//! @param void
//! @returns void
static void shutdown()
{
    // Disable sensor and I2C peripheral
    gpioSi7021Disable();
    i2cDeinit();
    // Remove Sleep Block on EM2/EM3 so timer waits put us back into EM3
    i2cEM2BlockEnd();
    nextState = startState;
}

//!
//! @brief Process the pending event depending on current state.
//! First, we check if the event to process is one of our defined
//! signals, i.e. an external signal. If not, we return immediately
//!
//! @param evt
//! @return eventHandled - true if successful, false otherwise
//!
bool schedulerMain( struct gecko_cmd_packet *evt )
{
    bool eventHandled = true;
    uint32_t eventToProcess = 0;
    if( BGLIB_MSG_ID( evt->header ) == gecko_evt_system_external_signal_id )
    {
        eventToProcess = evt->data.evt_system_external_signal.extsignals;
    }
    else
    {
        // not an event for our state machine
        eventHandled = false;
        return eventHandled;
    }

    if( !isConnected() && ( eventToProcess != EVENT_BT_CONNECTION_LOST ) )
    {
        // There is no open BT connection and the connection was not _just_ lost
        eventHandled = true;
        return eventHandled;
    }

    switch( currentState )
    {
        case STATE_SENSOR_OFF:
        {
            switch( eventToProcess )
            {
                case EVENT_MEASURE_TEMPERATURE:
                {
                    nextState = STATE_WAIT_FOR_POWERUP;
                    gpioSi7021Enable();
                    // wait 80ms for sensor power-up sequence
                    timerWaitUs( 80000 );

                    i2cInit();
                    break;
                }
                case EVENT_IDLE:
                {
                    nextState = currentState;
                    break;
                }
                case EVENT_BT_CONNECTION_LOST:
                {
                    shutdown();
                    break;
                }
                default:
                {
                    LOG_WARN( "Invalid event/state combination (%s)/(%s)",
                        getEventString( eventToProcess ), getStateString( currentState ) );
                    nextState = startState; // reset state machine
                    break;
                }
            }
            break;
        }
        case STATE_WAIT_FOR_POWERUP:
        {
            switch( eventToProcess )
            {
                case EVENT_LETIMER0_COMP1:
                {
                    i2cEM2BlockStart();
                    I2C_TransferReturn_TypeDef ret = i2cSendCommand();
                    if( i2cTransferInProgress == ret )
                    {
                        nextState = STATE_WAIT_FOR_I2C_WRITE;
                    }
                    break;
                }
                case EVENT_IDLE:
                {
                    nextState = currentState;
                    break;
                }
                case EVENT_BT_CONNECTION_LOST:
                {
                    shutdown();
                    break;
                }
                default:
                {
                    LOG_WARN( "Invalid event/state combination (%s)/(%s)",
                        getEventString( eventToProcess ), getStateString( currentState ) );
                    nextState = startState; // reset state machine
                    break;
                }
            }
            break;
        }
        case STATE_WAIT_FOR_I2C_WRITE:
        {
            switch( eventToProcess )
            {
                case EVENT_I2C_TRANSACTION_DONE:
                {
                    I2C_TransferReturn_TypeDef ret = i2cReceiveData();
                    if( i2cTransferInProgress == ret )
                    {
                        nextState = STATE_WAIT_FOR_I2C_READ;
                    }
                    break;
                }
                case EVENT_I2C_TRANSACTION_ERROR:
                {
                    shutdown();
                    LOG_ERROR( "%s in %s", getEventString( eventToProcess ), getStateString( currentState ) );
                    break;
                }
                case EVENT_IDLE:
                {
                    nextState = currentState;
                    break;
                }
                case EVENT_BT_CONNECTION_LOST:
                {
                    shutdown();
                    break;
                }
                default:
                {
                    LOG_WARN( "Invalid event/state combination (%s)/(%s)",
                        getEventString( eventToProcess ), getStateString( currentState ) );
                    nextState = startState; // reset state machine
                    break;
                }
            }
            break;
        }
        case STATE_WAIT_FOR_I2C_READ:
        {
            switch( eventToProcess )
            {
                case EVENT_I2C_TRANSACTION_DONE:
                {
                    shutdown();
                    // Convert raw data to degrees Celsius and log
                    i2cData_s *data = i2cGetDataBuffer();
                    // Buffer to store temperature data as a bitstream
                    uint8_t bitstreamBuffer[ 5 ];
                    // HTM flags set to 0 for Celsius, no timestamp and no temperature type
                    uint8_t flags = 0x00;
                    // Pointer to buffer needed to convert values to bitstream
                    uint8_t *p = bitstreamBuffer;
                    // Convert flags to bitstream and append to bitstreamBuffer
                    UINT8_TO_BITSTREAM( p, flags );
                    // Prepare temperature data to convert to bitstream
                    uint32_t temperature = FLT_TO_UINT32( data->temperature * 1000, -3 );
                    UINT32_TO_BITSTREAM( p, temperature );

                    if( isReadyForTemperature() )
                    {
                        // Send temperature indication to client
                        BTSTACK_CHECK_RESPONSE(
                            gecko_cmd_gatt_server_send_characteristic_notification(
                                getConnectionHandle(),   // Send to open connection
                                gattdb_temperature_measurement, // Temperature characteristic
                                5,  // Length of data to send in bytes
                                bitstreamBuffer ) );    // Bitstream buffer
                    }
                    LOG_TEMPERATURE( data->temperature );
                    displayPrintf( DISPLAY_ROW_TEMPVALUE, "Temp = %3.1f C", data->temperature );
                    break;
                }
                case EVENT_I2C_TRANSACTION_ERROR:
                {
                    shutdown();
                    LOG_ERROR( "%s in %s", getEventString( eventToProcess ), getStateString( currentState ) );
                    break;
                }
                case EVENT_IDLE:
                {
                    nextState = currentState;
                    break;
                }
                case EVENT_BT_CONNECTION_LOST:
                {
                    shutdown();
                    break;
                }
                default:
                {
                    LOG_WARN( "Invalid event/state combination (%s)/(%s)",
                        getEventString( eventToProcess ), getStateString( currentState ) );
                    nextState = startState; // reset state machine
                    break;
                }
            }
            break;
        }
        default:
        {
            eventHandled = false;
            break;
        }
    }
    if( currentState != nextState )
    {
        LOG_INFO( "Transition [%s]-->[%s]",
            getStateString( currentState ), getStateString( nextState ) );
        currentState = nextState;
    }

    return eventHandled;
}