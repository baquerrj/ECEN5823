//!
//! @file ble.h
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
//! @resources Utilized Silicon Labs' EMLIB peripheral libraries to implement functionality
//!
//! @copyright All rights reserved. Distribution allowed only for the use of assignment grading. Use of code excerpts allowed at the discretion of author. Contact for permission.
//!

#ifndef __BLE_H___
#define __BLE_H___

#include "native_gecko.h"
#include "ble_device_type.h"

//! Advertising interval given in units of (value * 0.625) ms, so 400 * 0.625 = 250ms advertising interval
static const uint16_t MAX_ADVERTISING_INTERVAL = 400;
//! Advertising interval given in units of (value * 0.625) ms, so 400 * 0.625 = 250ms advertising interval
static const uint16_t MIN_ADVERTISING_INTERVAL = 400;

static const uint8_t SCAN_TYPE = 1;

//! Scanning interval given in units of (value * 0.625) ms, so 64 * 0.625 = 40 ms scanning interval
static const uint16_t SCAN_INTERVAL = 64;
//static const uint16_t SCAN_INTERVAL = 400;
//! Scanning window given in units of (value * 0.625) ms, so 32 * 0.625 = 20m s scanning window
static const uint16_t SCAN_WINDOW = 32;
//static const uint16_t SCAN_WINDOW = 400;

//! Connection interval given in units of (value * 1.25) ms, so 60 * 1.25 = 75 ms connection interval
static const uint16_t MAX_CONNECTION_INTERVAL = 60;
//! Connection interval given in units of (value * 1.25) ms, so 60 * 1.25 = 75 ms connection interval
static const uint16_t MIN_CONNECTION_INTERVAL = 60;
//! Number of connection intervals slave is allowed to skip if no data is present
//! 4 * 75ms intervals = 300ms for slave latency
static const uint16_t SLAVE_LATENCY = 4;
//! Supervision timeout in milliseconds should be (1 + slave latency) * max_interval * 2
//! The math works out to a timeout of at least 750 ms, but the input is in
//! increments of 10 ms, so at least an input of value of 75 should work
static const uint16_t SUPERVISION_TIMEOUT = 150;

//! Maximum TX Power in steps of 0.1 dBm (10.5 dBm)
static const int16_t MAX_TX_POWER = 105;
//! Minimum TX Power in steps of 0.1 dBm (-30 dBm)
static const int16_t MIN_TX_POWER = -300;

typedef struct {
    uint8_t connection;
    uint32_t service;
    uint16_t characteristic;
} handles_s;

typedef struct {
    bd_addr address;
    uint8_t addressType;
} btAddress_s;

typedef struct {
    uint8_t data[ 2 ];
    uint8_t len;
} uuid_s;

typedef enum {
    GATT_IDLE = 0,
    GATT_WAITING_FOR_SERVICES_DISCOVERY,
    GATT_SERVICES_DISCOVERED,
    GATT_WAITING_FOR_CHARACTERISTICS_DISCOVERY,
    GATT_CHARACTERISTICS_DISCOVERED,
    GATT_WAITING_FOR_CHARACTERISTIC_VALUE,
    GATT_NUMBER_OF_STATES
} gattStates_e;

//! String representations for states
static const char *clientStateStrings[] = {
    "GATT_IDLE",
    "GATT_WAITING_FOR_SERVICES_DISCOVERY",
    "GATT_SERVICES_DISCOVERED",
    "GATT_WAITING_FOR_CHARACTERISTICS_DISCOVERY",
    "GATT_CHARACTERISTICS_DISCOVERED",
    "GATT_WAITING_FOR_CHARACTERISTIC_VALUE"
};

//! getClientStateString()
//! @brief Returns the string representation of the
//! input schedulerStates_e by indexing into clientStateStrings
//!
//! @param st
//! @returns string representation of st if valid state
static inline const char *getClientStateString( gattStates_e st )
{
    if( st < GATT_NUMBER_OF_STATES )
    {
        return clientStateStrings[ st ];
    }
    else
    {
        return "";
    }
}


int32_t gattFloat32ToInt( const uint8_t *value_start_little_endian );

float gattUint32ToFloat( const uint8_t *value_start_little_endian );

bool isReadyForTemperature();

bool isConnected();

uint8_t getConnectionHandle();

int16_t determineTxPower( int8_t rssi );

void setTxPower( int16_t power );

void handleSystemBootEvent();

bool handleServerEvent( struct gecko_cmd_packet *event );

bool handleClientEvent( struct gecko_cmd_packet *event );

static inline bool handleBleEvent( struct gecko_cmd_packet *event )
{
#if DEVICE_IS_BLE_SERVER == 1
    return handleServerEvent( event );
#else
    return handleClientEvent( event );
#endif
}


#endif // __BLE_H___
