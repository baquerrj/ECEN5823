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
//! @assignment ecen5823-assignment6-baquerrj
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
#include "display.h"

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
            displayPrintf( DISPLAY_ROW_NAME, "Server" );
            struct gecko_msg_system_get_bt_address_rsp_t *rsp;
            rsp = gecko_cmd_system_get_bt_address();
            displayPrintf( DISPLAY_ROW_BTADDR, "%X:%X:%X:%X:%X:%X",
                rsp->address.addr[ 0 ], rsp->address.addr[ 1 ], rsp->address.addr[ 2 ],
                rsp->address.addr[ 3 ], rsp->address.addr[ 4 ], rsp->address.addr[ 5 ] );
            displayPrintf( 	DISPLAY_ROW_CONNECTION, "Advertising" );
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
            displayPrintf( DISPLAY_ROW_CONNECTION, "Connected" );
            connectionHandle = event->data.evt_le_connection_opened.connection;
            deviceConnected = true;
            // Setting connection parameters
            BTSTACK_CHECK_RESPONSE(
                gecko_cmd_le_connection_set_parameters( event->data.evt_le_connection_opened.connection,
                    MIN_CONNECTION_INTERVAL,
                    MAX_CONNECTION_INTERVAL,
                    SLAVE_LATENCY,
                    SUPERVISION_TIMEOUT ) );

            struct gecko_msg_le_connection_opened_evt_t openedConnection;
            openedConnection = event->data.evt_le_connection_opened;

            LOG_DEBUG( "CONNECTION OPENED: address: %X:%X:%X:%X:%X:%X : address_type: %d : master: 0x%X : "
                "connection: 0x%X : bonding: 0x%X : advertiser: 0x%X",
                openedConnection.address.addr[ 0 ], openedConnection.address.addr[ 1 ], openedConnection.address.addr[ 2 ],
                openedConnection.address.addr[ 3 ], openedConnection.address.addr[ 4 ], openedConnection.address.addr[ 5 ],
                openedConnection.address_type,
                openedConnection.master,
                openedConnection.connection,
                openedConnection.bonding,
                openedConnection.advertiser );

            break;
        }
        case gecko_evt_le_connection_parameters_id:
        {
            struct gecko_msg_le_connection_parameters_evt_t params = event->data.evt_le_connection_parameters;
            if( connectionHandle != params.connection )
            {
                LOG_WARN( "Opened Connection /= Received Connection (%d/=%d)", connectionHandle, params.connection );
            }
            else
            {
                LOG_DEBUG( "CONNECTION PARAMETERS: connection handle: %d : connection interval: %d : slave latency: %d :"
                    "supervision timout: %d : security mode: %d : tx size: %d",
                    params.connection,
                    params.interval,
                    params.latency,
                    params.timeout,
                    params.security_mode,
                    params.txsize );
            }
            deviceConnected = true;
            break;
        }
        case gecko_evt_gatt_server_characteristic_status_id:
        {
            deviceConnected = true;
            struct gecko_msg_gatt_server_characteristic_status_evt_t characteristicStatus;
            characteristicStatus = event->data.evt_gatt_server_characteristic_status;
            LOG_DEBUG( "GATT SERVER STATUS: connection: 0x%X : characteristic: 0x%X : "
                "status_flags: 0x%X : client_config_flags: 0x%X",
                characteristicStatus.connection,
                characteristicStatus.characteristic,
                characteristicStatus.status_flags,
                characteristicStatus.client_config_flags );

            if( characteristicStatus.connection != connectionHandle )
            {
                LOG_WARN( "Opened Connection /= Received Connection (%d/=%d)", connectionHandle, characteristicStatus.connection );
                readyForTemperature = false;
                break;
            }
            // Determine if client is ready for temperature measurements by checking client_config_flags
            if( ( characteristicStatus.characteristic == gattdb_temperature_measurement ) &&
                ( characteristicStatus.status_flags == 0x01 ) )
            {
                if( characteristicStatus.client_config_flags == 0x02 )
                {
                    readyForTemperature = true;
                }
            }
            else if( characteristicStatus.client_config_flags == 0x00 )
            {
                readyForTemperature = false;
            }

            BTSTACK_CHECK_RESPONSE(
                gecko_cmd_le_connection_get_rssi( characteristicStatus.connection ) );
            break;
        }
        case gecko_evt_le_connection_rssi_id:
        {
            struct gecko_msg_le_connection_rssi_evt_t connectionRssi;
            connectionRssi = event->data.evt_le_connection_rssi;
            int8_t rssi = connectionRssi.rssi;
            LOG_DEBUG( "CONNECTION RSSI: connection: %d : status: %d : rssi: %d",
                connectionRssi.connection, connectionRssi.status, connectionRssi.rssi );
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
            displayPrintf( DISPLAY_ROW_CONNECTION, "Advertising" );
            displayPrintf( DISPLAY_ROW_TEMPVALUE, "Temp = ---- C" );
            struct gecko_msg_le_connection_closed_evt_t closedConnection;
            closedConnection = event->data.evt_le_connection_closed;
            if( connectionHandle == closedConnection.connection )
            {
                LOG_DEBUG( "CONNECTION CLOSED: connection: %d : reason: %d", closedConnection.connection, closedConnection.reason );
                // Reset connection handle
                connectionHandle = 0;
            }
            else
            {
                LOG_WARN( "Opened Connection /= Closed Connection (%d/=%d)", connectionHandle, closedConnection.connection );
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
            struct gecko_msg_le_connection_phy_status_evt_t phyStatus;
            phyStatus = event->data.evt_le_connection_phy_status;
            LOG_DEBUG( "PHY STATUS: connection: %d : phy: %d", phyStatus.connection, phyStatus.phy );
            break;
        }
        case gecko_evt_gatt_mtu_exchanged_id:
        {
            struct gecko_msg_gatt_mtu_exchanged_evt_t mtuExchanged;
            mtuExchanged = event->data.evt_gatt_mtu_exchanged;
            LOG_DEBUG( "MTU EXCHANED: connection: %d : mtu: %d", mtuExchanged.connection, mtuExchanged.mtu );
            break;
        }
        case gecko_evt_hardware_soft_timer_id:
        {
            // Don't need to #if ECEN5823_INCLUDE_DISPLAY_SUPPORT this part since when
            // ECEN5823_INCLUDE_DISPLAY_SUPPORT is 0, the display functions become no-ops
            displayUpdate();
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
