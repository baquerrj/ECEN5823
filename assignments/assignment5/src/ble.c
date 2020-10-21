//!
//! @file ble.c
//! @brief 
//! @version 0.1
//! 
//! @date 2020-09-27
//! @author Roberto Baquerizo (roba8460@colorado.edu)
//! 
//! @institution University of Colorado Boulder (UCB)
//! @course ECEN 5823-001: IoT Embedded Firmware (Fall 2020)
//! @instructor David Sluiter
//! 
//! @assignment ecen5823-assignment5-baquerrj
//! 
//! @resources Utilized Silicon Labs' EMLIB peripheral libraries to implement functionality @n
//! https://docs.silabs.com/resources/bluetooth/codeexamples/applicaBons/thermometer-example-with-efr32-internal-temperature-sensor/source/app.c
//! 
//! @copyright All rights reserved. Distribution allowed only for the use of assignment grading. Use of code excerpts allowed at the discretion of author. Contact for permission.
//!

#include "ble.h"

#include "gatt_db.h"
#include "gecko_ble_errors.h"
#include "log.h"
#include "scheduler.h"

#include <stdbool.h>

//! Flag indicating whether we have an open connection
static bool deviceConnected = false;

//! Connection handle for the current open connection
//! Set to evt_le_connection_opened.connection when connection is detected
static uint8_t connectionHandle = 0;

//! Flag indicating whether indications for temperature measurement have been turned on
static bool readyForTemperature = false;

//! isReadyForTemperature()
//! @brief Asserts whether clients is ready for server to transmit
//! a temperature measurement, i.e. it has turned on indications
//! 
//! @return true if ready
bool isReadyForTemperature()
{
    return readyForTemperature;
}

//! isConnected()
//! @brief Returns the connected status of the bluetooth device
//! 
//! @return true if there is an open connection
//! @return false if no open connection
bool isConnected()
{
    return deviceConnected;
}

//! getConnectionHandle()
//! @brief Get the Connection Handle object
//! 
//! @param void
//! @return connectionHandle
uint8_t getConnectionHandle()
{
    return connectionHandle;
}

//! setTxPower()
//! @brief Set the TX power for bluetooth in a safe way while @n
//! connection is open by: halting the system, calling BT API @n
//! to set the TX power, and resuming system operation
//! 
//! @param power 
//! @returns void
void setTxPower( int16_t power )
{
    BTSTACK_CHECK_RESPONSE( gecko_cmd_system_halt( 1 ) );
    // Set power
    struct gecko_msg_system_set_tx_power_rsp_t *rsp;
    rsp = gecko_cmd_system_set_tx_power( power );
    if( rsp->set_power != power )
    {
        LOG_WARN( "SET TX POWER: %.1f dBm : REQUESTED TX POWER: %.1f dBm)",
                rsp->set_power * .10, power * .10);
    }
    else
    {
        LOG_DEBUG( "TX POWER: %.1f dBm" , rsp->set_power * 0.10 );
    }
    BTSTACK_CHECK_RESPONSE( gecko_cmd_system_halt( 0 ) );
    return;
}

//! handleBleEvent()
//! @brief Handles any event from the Bluetooth Stack @n
//! For example, @ref gecko_evt_system_boot, gecko_evt_le_connection_opened_id
//! 
//! @param event 
//! @returns true if event was handled here, otherwise false @n
//! meaning that we encountered an external signal from our IRQs
bool handleBleEvent( struct gecko_cmd_packet *event )
{
    bool eventHandled = true;
    switch( BGLIB_MSG_ID( event->header ) )
    {
        case gecko_evt_system_boot_id:
        {
            LOG_INFO( "SYSTEM BOOT" );
            struct gecko_msg_le_gap_set_advertise_timing_rsp_t *set_advertise_timing_rsp;
            set_advertise_timing_rsp = BTSTACK_CHECK_RESPONSE(
                gecko_cmd_le_gap_set_advertise_timing(
                    0,
                    MIN_ADVERTISING_INTERVAL,
                    MAX_ADVERTISING_INTERVAL,
                    0,
                    0 ) );
            if( set_advertise_timing_rsp->result == bg_err_success )
            {
                struct gecko_msg_le_gap_start_advertising_rsp_t *start_advertising_rsp;
                // Start general advertising and enable connections
                start_advertising_rsp = BTSTACK_CHECK_RESPONSE(
                    gecko_cmd_le_gap_start_advertising(
                        0,
                        le_gap_general_discoverable,
                        le_gap_connectable_scannable ) );
                if( start_advertising_rsp->result != bg_err_success )
                {
                    LOG_WARN( "Error while starting advertising" );
                }
            }
            else
            {
                LOG_WARN( "Error while setting advertise timing" );
            }
            break;
        }
        case gecko_evt_system_external_signal_id:
        {
            eventHandled = false;
            break;
        }
        case gecko_evt_le_connection_opened_id:
        {
            LOG_INFO( "CONNECTIONED OPENED" );
            deviceConnected = true;
            struct gecko_msg_le_connection_set_parameters_rsp_t *set_parameters_rsp;
            // Setting connection parameters
            set_parameters_rsp = BTSTACK_CHECK_RESPONSE(
                gecko_cmd_le_connection_set_parameters( event->data.evt_le_connection_opened.connection,
                    MIN_CONNECTION_INTERVAL,
                    MAX_CONNECTION_INTERVAL,
                    SLAVE_LATENCY,
                    SUPERVISION_TIMEOUT ) );

            connectionHandle = event->data.evt_le_connection_opened.connection;
            if( set_parameters_rsp->result != bg_err_success )
            {
                LOG_ERROR( "SOME PARAMETERS REFUSED" );
            }
            break;
        }
        case gecko_evt_le_connection_parameters_id:
        {
            LOG_INFO( "PARAMETERS SET" );

            struct gecko_msg_le_connection_parameters_evt_t params = event->data.evt_le_connection_parameters;
            if( connectionHandle != params.connection )
            {
                LOG_WARN( "Opened Connection /= Received Connection (%d/=%d)", connectionHandle, params.connection );
            }
            else
            {
                LOG_DEBUG( "   CONNECTION HANDLE: %d", params.connection );
            }
            LOG_DEBUG( "   CONNECTION INTERVAL: %d", params.interval );
            LOG_DEBUG( "   SLAVE LATENCY: %d", params.latency );
            LOG_DEBUG( "   TIMEOUT: %d", params.timeout );
            LOG_DEBUG( "   SECURITY MODE: %d", params.security_mode );
            LOG_DEBUG( "   TXSIZE: %d", params.txsize );

            deviceConnected = true;
            break;
        }
        case gecko_evt_gatt_server_characteristic_status_id:
        {
            deviceConnected = true;
            LOG_INFO( "GATT SERVER STATUS" );
            // Determine if client is ready for temperature measurements by checking client_config_flags
            if( ( event->data.evt_gatt_server_characteristic_status.characteristic == gattdb_temperature_measurement ) &&
                ( event->data.evt_gatt_server_characteristic_status.status_flags == 0x01 ) )
            {
                if( event->data.evt_gatt_server_characteristic_status.client_config_flags == 0x02 )
                {
                    readyForTemperature = true;
                }
            }
            else if( event->data.evt_gatt_server_characteristic_status.client_config_flags == 0x00 )
            {
                readyForTemperature = false;
            }

            BTSTACK_CHECK_RESPONSE(
                gecko_cmd_le_connection_get_rssi( event->data.evt_gatt_server_characteristic_status.connection ) );
            break;
        }
        case gecko_evt_le_connection_rssi_id:
        {
            LOG_INFO( "CONNECTION RSSI" );
            int8_t rssi = event->data.evt_le_connection_rssi.rssi;
            LOG_DEBUG( "RSSI: %d", rssi );
            int16_t txPower = 0;
            if( rssi > -35 )
            {
                txPower = MIN_TX_POWER;
            }
            else if( ( rssi <= -35 ) && ( rssi > -45 ) )
            {
                txPower = -200; // -20 dBm
            }
            else if( ( rssi <= -45 ) && ( rssi > -55 ) )
            {
                txPower = -150; // -15 dBm
            }
            else if( ( rssi <= -55 ) && ( rssi > -65 ) )
            {
                txPower = -50; // -5 dBm
            }
            else if( ( rssi <= -65 ) && ( rssi > -75 ) )
            {
                txPower = 0; // 0 dBm
            }
            else if( ( rssi <= -75 ) && ( rssi > -85 ) )
            {
                txPower = 50; // 5 dBm
            }
            else if( rssi <= -85 )
            {
                txPower = MAX_TX_POWER;
            }

            setTxPower( txPower );
            break;
        }
        case gecko_evt_le_connection_closed_id:
        {
            LOG_INFO( "CONNECTION CLOSED" );

            uint8_t closedConnection = event->data.evt_le_connection_closed.connection;
            if( connectionHandle == closedConnection )
            {
                // Reset connection handle
                connectionHandle = 0;
            }
            else
            {
                LOG_WARN( "Opened Connection /= Closed Connection (%d/=%d)", connectionHandle, closedConnection );
            }
            setTxPower( 0 );
            BTSTACK_CHECK_RESPONSE(
                gecko_cmd_le_gap_start_advertising(
                    0,
                    le_gap_general_discoverable,
                    le_gap_connectable_scannable ) );

            schedulerSetEventConnectionLost();
            deviceConnected = false;
            break;
        }
        case gecko_evt_le_connection_phy_status_id:
        {
            LOG_INFO( "PHY STATUS" );
            break;
        }
        case gecko_evt_gatt_mtu_exchanged_id:
        {
            LOG_INFO( "MTU EXCHANGED" );
            break;
        }
        default:
        {
            LOG_WARN( "UNKNOWN EVENT ENCOUNTERED [0x%lx]", BGLIB_MSG_ID( event->header ) );
            eventHandled = false;
            break;
        }
    }
    return eventHandled;
}

