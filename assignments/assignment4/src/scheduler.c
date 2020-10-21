//!
//! @file scheduler.c
//! @brief Implementation of scheduler as a state machine to handle events triggered by interrupts
//! @version 0.1
//! 
//! @date 2020-09-24
//! @author Roberto Baquerizo (roba8460@colorado.edu)
//! 
//! @instiution University of Colorado Boulder (UCB)
//! @course ECEN 5823-001: IoT Embedded Firmware (Fall 2020)
//! @instructor David Sluiter
//! 
//! @assignment ecen5823-assignment4-baquerrj
//! 
//! @resources Utilized Silicon Labs' EMLIB peripheral libraries to implement functionality
//! 
//! @copyright All rights reserved. Distribution allowed only for the use of assignment grading. Use of code excerpts allowed at the discretion of author. Contact for permission.
//!

#include "scheduler.h"

#include "main.h"
#include "log.h"
#include "gpio.h"
#include "i2c.h"
#include "timers.h"

#include "em_core.h"

 //! Used for signaling events
static events_e event;
//! Stores the current state of the scheduler's state machine
static states_e currentState;
//! Stores the state the state machine will transitition to
static states_e nextState;

void schedulerInit()
{
    event = EVENT_IDLE;
    currentState = STATE_SENSOR_OFF;
    nextState = STATE_WAIT_FOR_POWERUP;

    LOG_DEBUG( "exiting" );
    return;
}

//! schedulerMain()
//! @brief Process the pending event depending on current state
//!
//! @paramp _event event to process
//! @returns true if successful, false if encountered an error
bool schedulerMain( events_e _event )
{
    events_e eventToProcess = _event;
    bool status = true;
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
                default:
                {
                    LOG_WARN( "Invalid event/state combination (%s)/(%s)",
                        getEventString( eventToProcess ), getStateString( currentState ) );
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
                default:
                {
                    LOG_WARN( "Invalid event/state combination (%s)/(%s)",
                        getEventString( eventToProcess ), getStateString( currentState ) );
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
                    // Disable sensor and I2C peripheral
                    gpioSi7021Disable();
                    i2cDeinit();
                    // Remove Sleep Block on EM2/EM3 so timer waits put us back into EM3
                    i2cEM2BlockEnd();
                    LOG_ERROR( "%s in %s", getEventString( eventToProcess ), getStateString( currentState ) );
                    nextState = STATE_SENSOR_OFF;
                    break;
                }
                case EVENT_IDLE:
                {
                    nextState = currentState;
                    break;
                }
                default:
                {
                    LOG_WARN( "Invalid event/state combination (%s)/(%s)",
                        getEventString( eventToProcess ), getStateString( currentState ) );
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
                    // Disable sensor and I2C peripheral
                    gpioSi7021Disable();
                    i2cDeinit();
                    // Remove Sleep Block on EM2/EM3 so timer waits put us back into EM3
                    i2cEM2BlockEnd();

                    // Convert raw data to degrees Celsius and log
                    i2cData_s* data = i2cGetDataBuffer();
                    LOG_TEMPERATURE( data->temperature );

                    nextState = STATE_SENSOR_OFF;
                    break;
                }
                case EVENT_I2C_TRANSACTION_ERROR:
                {
                    // Disable sensor and I2C peripheral
                    gpioSi7021Disable();
                    i2cDeinit();
                    // Remove Sleep Block on EM2/EM3 so timer waits put us back into EM3
                    i2cEM2BlockEnd();
                    LOG_ERROR( "%s in %s", getEventString( eventToProcess ), getStateString( currentState ) );
                    nextState = STATE_SENSOR_OFF;
                    break;
                }
                case EVENT_IDLE:
                {
                    nextState = currentState;
                    break;
                }
                default:
                {
                    LOG_WARN( "Invalid event/state combination (%s)/(%s)",
                        getEventString( eventToProcess ), getStateString( currentState ) );
                    break;
                }
            }
            break;
        }
        default:
        {
            LOG_WARN( "Unknown event detected [%d]", eventToProcess );
            status = false;
            break;
        }
    }
    if( currentState != nextState )
    {
        LOG_INFO( "Transition [%s]-->[%s]",
            getStateString( currentState ), getStateString( nextState ) );
        currentState = nextState;
    }

    return status;
}

//! schedulerGetEvent()
//! @brief Returns event containing events to be processed and clears it
//! When clearing events, disable interrupts so the operation is atomic
//!
//! @param void
//! @returns event
uint8_t schedulerGetEvent()
{
    uint8_t _event = 0;
    _event = event;
    schedulerClearEvent();
    return _event;
}

//! schedulerSetEventMeasureTemperature()
//! @brief Set event to EVENT_MEASURE_TEMPERATURE, only if there is no pending event
//! Always called from an interrupt context, so don't want to call
//! CORE_ATOMIC_IRQ_DISABLE() or CORE_ATOMIC_IRQ_ENABLE() inside here
//!
//! @param void
//! @returns void
void schedulerSetEventMeasureTemperature()
{
    if( event == EVENT_IDLE )
    {
        event = EVENT_MEASURE_TEMPERATURE;
    }
    return;
}

//! schedulerSetEventTimerDone()
//! @brief Set event to EVENT_LETIMER0_COMP1, only if there is no pending event
//! Always called from an interrupt context, so don't want to call
//! CORE_ATOMIC_IRQ_DISABLE() or CORE_ATOMIC_IRQ_ENABLE() inside here
//!
//! @param void
//! @returns void
void schedulerSetEventTimerDone()
{
    if( event == EVENT_IDLE )
    {
        event = EVENT_LETIMER0_COMP1;
    }
    return;
}

//! schedulerSetEventTransactionDone()
//! @brief Set event to EVENT_I2C_TRANSACTION_DONE, only if there is no pending event
//! Always called from an interrupt context, so don't want to call
//! CORE_ATOMIC_IRQ_DISABLE() or CORE_ATOMIC_IRQ_ENABLE() inside here
//!
//! @param void
//! @returns void
void schedulerSetEventTransactionDone()
{
    if( event == EVENT_IDLE )
    {
        event = EVENT_I2C_TRANSACTION_DONE;
    }
    return;
}

//! schedulerSetEventTransactionError()
//! @brief Set event to EVENT_I2C_TRANSACTION_ERROR, only if there is no pending event
//! Always called from an interrupt context, so don't want to call
//! CORE_ATOMIC_IRQ_DISABLE() or CORE_ATOMIC_IRQ_ENABLE() inside here
//!
//! @param void
//! @returns void
void schedulerSetEventTransactionError()
{
    if( event == EVENT_IDLE )
    {
        event = EVENT_I2C_TRANSACTION_ERROR;
    }
    return;
}

//! schedulerClearEvent()
//! @brief Clear any pending events by setting event to EVENT_IDLE
//!
//! @param void
//! @returns void
void schedulerClearEvent()
{
    CORE_ATOMIC_IRQ_DISABLE();
    event = EVENT_IDLE;
    CORE_ATOMIC_IRQ_ENABLE();
    return;
}

//! eventsPresent()
//! @brief Reports whether there any events to process
//!
//! @param void
//! @returns true if there are events to process, false otherwise
bool eventsPresent()
{
    return ( ( event > EVENT_IDLE ) ? true : false );
}

