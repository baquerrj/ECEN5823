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
//! @assignment ecen5823-assignment7-baquerrj
//!
//! @resources Utilized Silicon Labs' EMLIB peripheral libraries to implement functionality @n
//! https://docs.silabs.com/resources/bluetooth/codeexamples/applicaBons/thermometer-example-with-efr32-internal-temperature-sensor/source/app.c
//!
//! @copyright All rights reserved. Distribution allowed only for the use of assignment grading. Use of code excerpts allowed at the discretion of author. Contact for permission.
//!

#include "ble.h"
#include "log.h"
#include "scheduler.h"
#include "display.h"

#include "gatt_db.h"
#include "ble_device_type.h"
#include "gecko_ble_errors.h"

#include <stdbool.h>
#include <math.h>

static volatile uint32_t passkey = 0;

static gattStates_e currentClientState = GATT_IDLE;

static const bd_addr serverAddress = SERVER_BT_ADDRESS;

static handles_s handles = { 0, 0, 0 };

static btAddress_s advertiser = {};

static uuid_s htmService =
{
    .data = {0x09, 0x18},
    .len = 2
};

static uuid_s htmCharacteristic =
{
    .data = {0x1C, 0x2A},
    .len = 2
};

//! Flag indicating whether we have an open connection
static bool deviceConnected = false;

//! Flag indicating whether indications for temperature measurement have been turned on
static bool readyForTemperature = false;

// Original code from Dan Walkes. I (Sluiter) fixed a sign extension bug with the mantissa.
// convert IEEE-11073 32-bit float to integer
int32_t gattFloat32ToInt( const uint8_t *value_start_little_endian )
{
    uint8_t signByte = 0;
    int32_t mantissa;
    // data format pointed at by value_start_little_endian is:
    // [0] = contains the flags byte
    // [3][2][1] = mantissa (24-bit 2's complement)
    // [4] = exponent (8-bit 2's complement)
    int8_t exponent = ( int8_t ) value_start_little_endian[ 4 ];
    // sign extend the mantissa value if the mantissa is negative
    if( value_start_little_endian[ 3 ] & 0x80 )
    { // msb of [3] is the sign of the mantissa
        signByte = 0xFF;
    }
    mantissa = ( int32_t ) ( value_start_little_endian[ 1 ] << 0 ) |
        ( value_start_little_endian[ 2 ] << 8 ) |
        ( value_start_little_endian[ 3 ] << 16 ) |
        ( signByte << 24 );
        // value = 10^exponent * mantissa, pow() returns a double type
    return ( int32_t ) ( pow( 10, exponent ) * mantissa );
} // gattFloat32ToInt

float gattUint32ToFloat( const uint8_t *value_start_little_endian )
{
    uint8_t signByte = 0;
    int32_t mantissa;
    // data format pointed at by value_start_little_endian is:
    // [0] = contains the flags byte
    // [3][2][1] = mantissa (24-bit 2's complement)
    // [4] = exponent (8-bit 2's complement)
    int8_t exponent = ( int8_t ) value_start_little_endian[ 4 ];
    // sign extend the mantissa value if the mantissa is negative
    if( value_start_little_endian[ 3 ] & 0x80 )
    { // msb of [3] is the sign of the mantissa
        signByte = 0xFF;
    }
    mantissa = ( int32_t ) ( value_start_little_endian[ 1 ] << 0 ) |
        ( value_start_little_endian[ 2 ] << 8 ) |
        ( value_start_little_endian[ 3 ] << 16 ) |
        ( signByte << 24 );
        // value = 10^exponent * mantissa, pow() returns a double type
    return ( float ) ( pow( 10, exponent ) * mantissa );
}

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
    return handles.connection;
}

//!
//! @brief Determines tx power based on rssi value received
//!
//! @param rssi
//! @return int16_t
//!
int16_t determineTxPower( int8_t rssi )
{
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
    return txPower;
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
            rsp->set_power * .10, power * .10 );
    }
    else
    {
        LOG_DEBUG( "TX POWER: %.1f dBm", rsp->set_power * 0.10 );
    }
    BTSTACK_CHECK_RESPONSE( gecko_cmd_system_halt( 0 ) );
    return;
}

//!
//! @brief
//!
//! @param void
//!
void handleSystemBootEvent()
{
    displayPrintf( DISPLAY_ROW_NAME, BLE_DEVICE_TYPE_STRING );
    struct gecko_msg_system_get_bt_address_rsp_t *rsp;
    rsp = gecko_cmd_system_get_bt_address();
    displayPrintf( DISPLAY_ROW_BTADDR, "%X:%X:%X:%X:%X:%X",
        rsp->address.addr[ 0 ],
        rsp->address.addr[ 1 ],
        rsp->address.addr[ 2 ],
        rsp->address.addr[ 3 ],
        rsp->address.addr[ 4 ],
        rsp->address.addr[ 5 ] );
}

//! handleServerEvent()
//! @brief Handles any event from the Bluetooth Stack for Server funcionality @n
//! For example, @ref gecko_evt_system_boot, gecko_evt_le_connection_opened_id
//!
//! @param evt
//! @returns true if event was handled here, otherwise false @n
//! meaning that we encountered an external signal from our IRQs
bool handleServerEvent( struct gecko_cmd_packet *evt )
{
    bool eventHandled = true;
    switch( BGLIB_MSG_ID( evt->header ) )
    {
        case gecko_evt_system_boot_id:
        {
            handleSystemBootEvent();
            displayPrintf( DISPLAY_ROW_CONNECTION, "Advertising" );

            BTSTACK_CHECK_RESPONSE( gecko_cmd_sm_delete_bondings() );
            BTSTACK_CHECK_RESPONSE( gecko_cmd_flash_ps_erase_all() );

            // Configure SM to use MITM protection and display yes/no IO capabilities
            BTSTACK_CHECK_RESPONSE( gecko_cmd_sm_configure( 0x01, sm_io_capability_displayyesno ) );

            // Set bondable mode to accept new bondings
            BTSTACK_CHECK_RESPONSE( gecko_cmd_sm_set_bondable_mode( 1 ) );
            BTSTACK_CHECK_RESPONSE( gecko_cmd_le_gap_set_advertise_timing(
                0,
                MIN_ADVERTISING_INTERVAL,
                MAX_ADVERTISING_INTERVAL,
                0,
                0 ) );

            // Start general advertising and enable connections
            BTSTACK_CHECK_RESPONSE( gecko_cmd_le_gap_start_advertising(
                0,
                le_gap_general_discoverable,
                le_gap_connectable_scannable ) );
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
            handles.connection = evt->data.evt_le_connection_opened.connection;
            deviceConnected = true;
            // Setting connection parameters
            BTSTACK_CHECK_RESPONSE(
                gecko_cmd_le_connection_set_parameters( evt->data.evt_le_connection_opened.connection,
                    MIN_CONNECTION_INTERVAL,
                    MAX_CONNECTION_INTERVAL,
                    SLAVE_LATENCY,
                    SUPERVISION_TIMEOUT ) );

            LOG_INFO( "CONNECTION OPENED: address: %X:%X:%X:%X:%X:%X : address_type: %d : master: 0x%X : "
                "connection: 0x%X : bonding: 0x%X : advertiser: 0x%X",
                evt->data.evt_le_connection_opened.address.addr[ 0 ],
                evt->data.evt_le_connection_opened.address.addr[ 1 ],
                evt->data.evt_le_connection_opened.address.addr[ 2 ],
                evt->data.evt_le_connection_opened.address.addr[ 3 ],
                evt->data.evt_le_connection_opened.address.addr[ 4 ],
                evt->data.evt_le_connection_opened.address.addr[ 5 ],
                evt->data.evt_le_connection_opened.address_type,
                evt->data.evt_le_connection_opened.master,
                evt->data.evt_le_connection_opened.connection,
                evt->data.evt_le_connection_opened.bonding,
                evt->data.evt_le_connection_opened.advertiser );
            break;
        }
        case gecko_evt_le_connection_parameters_id:
        {
            if( handles.connection != evt->data.evt_le_connection_parameters.connection )
            {
                LOG_WARN( "Opened Connection /= Received Connection (%d/=%d)",
                    handles.connection,
                    evt->data.evt_le_connection_parameters.connection );
            }
            else
            {
                LOG_INFO( "CONNECTION PARAMETERS: connection handle: %d : connection interval: %d : slave latency: %d :"
                    "supervision timout: %d : security mode: %d : tx size: %d",
                    evt->data.evt_le_connection_parameters.connection,
                    evt->data.evt_le_connection_parameters.interval,
                    evt->data.evt_le_connection_parameters.latency,
                    evt->data.evt_le_connection_parameters.timeout,
                    evt->data.evt_le_connection_parameters.security_mode,
                    evt->data.evt_le_connection_parameters.txsize );
            }
            deviceConnected = true;
            break;
        }
        case gecko_evt_gatt_server_characteristic_status_id:
        {
            deviceConnected = true;
            LOG_DEBUG( "GATT SERVER STATUS: connection: 0x%X : characteristic: 0x%X : "
                "status_flags: 0x%X : client_config_flags: 0x%X",
                evt->data.evt_gatt_server_characteristic_status.connection,
                evt->data.evt_gatt_server_characteristic_status.characteristic,
                evt->data.evt_gatt_server_characteristic_status.status_flags,
                evt->data.evt_gatt_server_characteristic_status.client_config_flags );

            if( evt->data.evt_gatt_server_characteristic_status.connection != handles.connection )
            {
                LOG_WARN( "Opened Connection /= Received Connection (%d/=%d)",
                    handles.connection,
                    evt->data.evt_gatt_server_characteristic_status.connection );
                readyForTemperature = false;
                break;
            }
            // Determine if client is ready for temperature measurements by checking client_config_flags
            if( ( evt->data.evt_gatt_server_characteristic_status.characteristic == gattdb_temperature_measurement ) &&
                ( evt->data.evt_gatt_server_characteristic_status.status_flags == gatt_server_client_config ) )
            {
                if( evt->data.evt_gatt_server_characteristic_status.client_config_flags == gatt_indication )
                {
                    readyForTemperature = true;
                }
            }
            else if( evt->data.evt_gatt_server_characteristic_status.client_config_flags == gatt_disable )
            {
                readyForTemperature = false;
            }

            BTSTACK_CHECK_RESPONSE(
                gecko_cmd_le_connection_get_rssi( evt->data.evt_gatt_server_characteristic_status.connection ) );
            break;
        }
        case gecko_evt_le_connection_rssi_id:
        {
            LOG_INFO( "CONNECTION RSSI: connection: %d : status: %d : rssi: %d",
                evt->data.evt_le_connection_rssi.connection,
                evt->data.evt_le_connection_rssi.status,
                evt->data.evt_le_connection_rssi.rssi );
            int16_t txPower = determineTxPower( evt->data.evt_le_connection_rssi.rssi );
            setTxPower( txPower );
            break;
        }
        case gecko_evt_le_connection_closed_id:
        {
            displayPrintf( DISPLAY_ROW_CONNECTION, "Advertising" );
            displayPrintf( DISPLAY_ROW_TEMPVALUE, "Temp = ---- C" );
            if( handles.connection == evt->data.evt_le_connection_closed.connection )
            {
                LOG_DEBUG( "CONNECTION CLOSED: connection: %d : reason: %d",
                    evt->data.evt_le_connection_closed.connection,
                    evt->data.evt_le_connection_closed.reason );
                // Reset connection handle
                handles.connection = 0;
            }
            else
            {
                LOG_WARN( "Opened Connection /= Closed Connection (%d/=%d)",
                    handles.connection,
                    evt->data.evt_le_connection_closed.connection );
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
        case gecko_evt_sm_confirm_passkey_id:
        {
            passkey = evt->data.evt_sm_confirm_passkey.passkey;
            displayPrintf( DISPLAY_ROW_PASSKEY, "Passkey: %4lu", passkey );
            break;
        }
        case gecko_evt_sm_bonded_id:
        {
            displayPrintf( DISPLAY_ROW_CONNECTION, "Bonded" );
            break;
        }
        case gecko_evt_sm_bonding_failed_id:
        {
            displayPrintf( DISPLAY_ROW_CONNECTION, "Bonding Failed" );
            break;
        }
        case gecko_evt_le_connection_phy_status_id:
        {
            LOG_DEBUG( "PHY STATUS: connection: %d : phy: %d",
                evt->data.evt_le_connection_phy_status.connection,
                evt->data.evt_le_connection_phy_status.phy );
            break;
        }
        case gecko_evt_gatt_mtu_exchanged_id:
        {
            LOG_DEBUG( "MTU EXCHANED: connection: %d : mtu: %d",
                evt->data.evt_gatt_mtu_exchanged.connection,
                evt->data.evt_gatt_mtu_exchanged.mtu );
            break;
        }
        case gecko_evt_hardware_soft_timer_id:
        {
            displayUpdate();
            break;
        }
        default:
        {
            LOG_WARN( "UNKNOWN EVENT ENCOUNTERED [0x%lX]", BGLIB_MSG_ID( evt->header ) );
            eventHandled = false;
            break;
        }
    }
    return eventHandled;
}

//! handleClientEvent()
//! @brief Handles any event from the BT stack for Client functionality @n
//! E.g. start discovery of advertising devices, connect to discovered device,
//! start discovery of services and characteristics, enable indications
//!
//! @param evt
//! @returns true if event was handled, otherwise false
bool handleClientEvent( struct gecko_cmd_packet *evt )
{
    gattStates_e nextClientState = currentClientState;
    bool eventHandled = true;
    switch( BGLIB_MSG_ID( evt->header ) )
    {
        case gecko_evt_system_boot_id:
        {
            handleSystemBootEvent( evt );
            displayPrintf( DISPLAY_ROW_CONNECTION, "Discovering" );

            BTSTACK_CHECK_RESPONSE( gecko_cmd_le_gap_set_discovery_type(
                le_gap_phy_1m,
                SCAN_TYPE
            ) );

            BTSTACK_CHECK_RESPONSE( gecko_cmd_le_gap_set_discovery_timing(
                le_gap_phy_1m,
                SCAN_INTERVAL,
                SCAN_WINDOW
            ) );

            BTSTACK_CHECK_RESPONSE( gecko_cmd_le_gap_start_discovery(
                le_gap_phy_1m,
                le_gap_general_discoverable
            ) );
            break;
        }
        case gecko_evt_le_gap_scan_response_id:
        {
            advertiser.address = evt->data.evt_le_gap_scan_response.address;
            advertiser.addressType = evt->data.evt_le_gap_scan_response.address_type;

            if( ( advertiser.address.addr[ 0 ] == serverAddress.addr[ 0 ] ) &&
                ( advertiser.address.addr[ 1 ] == serverAddress.addr[ 1 ] ) &&
                ( advertiser.address.addr[ 2 ] == serverAddress.addr[ 2 ] ) &&
                ( advertiser.address.addr[ 3 ] == serverAddress.addr[ 3 ] ) &&
                ( advertiser.address.addr[ 4 ] == serverAddress.addr[ 4 ] ) &&
                ( advertiser.address.addr[ 5 ] == serverAddress.addr[ 5 ] ) )
            {
                displayPrintf( DISPLAY_ROW_BTADDR2, "%X:%X:%X:%X:%X:%X",
                    advertiser.address.addr[ 0 ], advertiser.address.addr[ 1 ], advertiser.address.addr[ 2 ],
                    advertiser.address.addr[ 3 ], advertiser.address.addr[ 4 ], advertiser.address.addr[ 5 ] );

                BTSTACK_CHECK_RESPONSE( gecko_cmd_le_gap_end_procedure() );

                BTSTACK_CHECK_RESPONSE( gecko_cmd_le_gap_connect(
                    advertiser.address,
                    advertiser.addressType,
                    le_gap_phy_1m
                ) );
            }
            break;
        }
        case gecko_evt_le_connection_opened_id:
        {
            if( currentClientState == GATT_IDLE )
            {
                nextClientState = GATT_WAITING_FOR_SERVICES_DISCOVERY;
                displayPrintf( DISPLAY_ROW_CONNECTION, "Connected" );
            }
            handles.connection = evt->data.evt_le_connection_opened.connection;
            BTSTACK_CHECK_RESPONSE( gecko_cmd_le_connection_set_parameters(
                handles.connection,
                MIN_CONNECTION_INTERVAL,
                MAX_CONNECTION_INTERVAL,
                SLAVE_LATENCY,
                SUPERVISION_TIMEOUT
            ) );

            BTSTACK_CHECK_RESPONSE( gecko_cmd_gatt_discover_primary_services_by_uuid(
                handles.connection,
                htmService.len,
                htmService.data
            ) );

            if( currentClientState == GATT_IDLE )
            {
                nextClientState = GATT_WAITING_FOR_SERVICES_DISCOVERY;
            }

            break;
        }
        case gecko_evt_le_connection_parameters_id:
        {
            if( handles.connection != evt->data.evt_le_connection_parameters.connection )
            {
                LOG_WARN( "Opened Connection /= Received Connection (%d/=%d)",
                    handles.connection,
                    evt->data.evt_le_connection_parameters.connection );
            }
            else
            {
                LOG_INFO( "CONNECTION PARAMETERS: connection handle: %d : connection interval: %d : slave latency: %d :"
                    "supervision timout: %d : security mode: %d : tx size: %d",
                    evt->data.evt_le_connection_parameters.connection,
                    evt->data.evt_le_connection_parameters.interval,
                    evt->data.evt_le_connection_parameters.latency,
                    evt->data.evt_le_connection_parameters.timeout,
                    evt->data.evt_le_connection_parameters.security_mode,
                    evt->data.evt_le_connection_parameters.txsize );
            }
            break;
        }
        case gecko_evt_gatt_service_id:
        {
            if( currentClientState == GATT_WAITING_FOR_SERVICES_DISCOVERY )
            {
                nextClientState = GATT_SERVICES_DISCOVERED;
            }
            handles.service = evt->data.evt_gatt_service.service;
            if( 0 == memcmp( evt->data.evt_gatt_service.uuid.data, htmService.data, htmService.len ) )
            {
                LOG_INFO( "GATT Service: service: 0x%lX : uuid: 0x%02X%02X",
                    evt->data.evt_gatt_service.service,
                    evt->data.evt_gatt_service.uuid.data[ 0 ],
                    evt->data.evt_gatt_service.uuid.data[ 1 ] );
            }
            break;
        }
        case gecko_evt_gatt_characteristic_id:
        {
            if( currentClientState == GATT_WAITING_FOR_CHARACTERISTICS_DISCOVERY )
            {
                nextClientState = GATT_CHARACTERISTICS_DISCOVERED;
            }
            handles.characteristic = evt->data.evt_gatt_characteristic.characteristic;
            if( 0 == memcmp( evt->data.evt_gatt_characteristic.uuid.data, htmCharacteristic.data, htmCharacteristic.len ) )
            {
                LOG_INFO( "GATT Characteristic: characteristic: 0x%04X : uuid: 0x%02X%02X",
                    evt->data.evt_gatt_characteristic.characteristic,
                    evt->data.evt_gatt_characteristic.uuid.data[ 1 ],
                    evt->data.evt_gatt_characteristic.uuid.data[ 0 ] );
            }
            break;
        }
        case gecko_evt_gatt_characteristic_value_id:
        {
            if( currentClientState == GATT_WAITING_FOR_CHARACTERISTIC_VALUE )
            {
                nextClientState = GATT_IDLE;
                displayPrintf( DISPLAY_ROW_CONNECTION, "Handling Indications" );
            }

            if( evt->data.evt_gatt_characteristic_value.att_opcode == gatt_handle_value_indication )
            {
                BTSTACK_CHECK_RESPONSE( gecko_cmd_gatt_send_characteristic_confirmation( handles.connection ) );
            }

            if( evt->data.evt_gatt_characteristic_value.characteristic == gattdb_temperature_measurement )
            {
                uint8_t *p = evt->data.evt_gatt_characteristic_value.value.data;
                float ftemp = gattUint32ToFloat( p );
                displayPrintf( DISPLAY_ROW_TEMPVALUE, "Temp = %3.1f C", ftemp );
            }

            BTSTACK_CHECK_RESPONSE(
                gecko_cmd_le_connection_get_rssi( handles.connection ) );

            break;
        }
        case gecko_evt_gatt_procedure_completed_id:
        {
            if( evt->data.evt_gatt_procedure_completed.result != bg_err_success )
            {
                LOG_WARN( "GATT Procedure Completed: connection: 0x%x : result: %s",
                    evt->data.evt_gatt_procedure_completed.connection,
                    bleResponseString( evt->data.evt_gatt_procedure_completed.result ) );
            }
            switch( currentClientState )
            {
                case GATT_SERVICES_DISCOVERED:
                {
                    handles.connection = evt->data.evt_gatt_procedure_completed.connection;

                    BTSTACK_CHECK_RESPONSE( gecko_cmd_gatt_discover_characteristics_by_uuid(
                        handles.connection,
                        handles.service,
                        htmCharacteristic.len,
                        htmCharacteristic.data
                    ) );

                    nextClientState = GATT_WAITING_FOR_CHARACTERISTICS_DISCOVERY;
                    break;
                }
                case GATT_CHARACTERISTICS_DISCOVERED:
                {
                    BTSTACK_CHECK_RESPONSE( gecko_cmd_gatt_set_characteristic_notification(
                        handles.connection,
                        handles.characteristic,
                        gatt_indication
                    ) );
                    nextClientState = GATT_WAITING_FOR_CHARACTERISTIC_VALUE;
                    break;
                }
                case GATT_WAITING_FOR_CHARACTERISTIC_VALUE:
                {
                    // Remain in this state until gecko_evt_gatt_characteristic_value_id event occurs
                    // If timeout occurs, gecko_evt_le_connection_closed_id is triggered by the BT stack
                    // and the state machine is reset
                    nextClientState = currentClientState;
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case gecko_evt_le_connection_closed_id:
        {
            displayPrintf( DISPLAY_ROW_BTADDR2, " " );
            displayPrintf( DISPLAY_ROW_CONNECTION, "Discovering" );
            displayPrintf( DISPLAY_ROW_TEMPVALUE, "" );

            nextClientState = GATT_IDLE;
            handles.connection = 0;
            handles.service = 0;
            handles.characteristic = 0;
            BTSTACK_CHECK_RESPONSE( gecko_cmd_le_gap_start_discovery(
                le_gap_phy_1m,
                le_gap_general_discoverable
            ) );
            break;
        }
        case gecko_evt_le_connection_rssi_id:
        {
            LOG_INFO( "CONNECTION RSSI: connection: %d : status: %d : rssi: %d",
                evt->data.evt_le_connection_rssi.connection,
                evt->data.evt_le_connection_rssi.status,
                evt->data.evt_le_connection_rssi.rssi );
            int16_t txPower = determineTxPower( evt->data.evt_le_connection_rssi.rssi );
            setTxPower( txPower );
            break;
        }
        case gecko_evt_hardware_soft_timer_id:
        {
            displayUpdate();
            break;
        }
        default:
            break;
    }
    if( currentClientState != nextClientState )
    {
        currentClientState = nextClientState;
    }

    return eventHandled;
}
