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
//! @assignment ecen5823-assignment4-baquerrj
//! 
//! @resources Utilized Silicon Labs' EMLIB peripheral libraries to implement functionality
//! 
//! @copyright All rights reserved. Distribution allowed only for the use of assignment grading. Use of code excerpts allowed at the discretion of author. Contact for permission.
//!

#ifndef __BLE_H___
#define __BLE_H___

#include "native_gecko.h"

//! Advertising interval given in units of (value * 1.6) ms, so 400 * 0.625 = 250ms advertising interval
static const uint16_t MAX_ADVERTISING_INTERVAL = 400;
//! Advertising interval given in units of (value * 1.6) ms, so 400 * 0.625 = 250ms advertising interval
static const uint16_t MIN_ADVERTISING_INTERVAL = 400;

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

bool isReadyForTemperature();

bool isConnected();

uint8_t getConnectionHandle();

void setTxPower( int16_t power );

bool handleBleEvent( struct gecko_cmd_packet *event );

#endif // __BLE_H___
