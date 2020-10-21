//!
//! @file scheduler.h
//! @brief Functions and type definitions for our sheduler
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

#ifndef __SCHEDULER_H___
#define __SCHEDULER_H___

#include <stdint.h>
#include <stdbool.h>
#include "native_gecko.h"
#include "em_core.h"
#include "ble_device_type.h"

//! Enum defining possible events that scheduler can process
typedef enum
{
    EVENT_IDLE,
    EVENT_MEASURE_TEMPERATURE,
    EVENT_LETIMER0_COMP1,
    EVENT_I2C_TRANSACTION_DONE,
    EVENT_I2C_TRANSACTION_ERROR,
    EVENT_BT_CONNECTION_LOST,
    NUMBER_OF_EVENTS
} schedulerEvents_e;

//! String representations for events
static const char *eventStrings[] = {
    "EVENT_IDLE",
    "EVENT_MEASURE_TEMPERATURE",
    "EVENT_LETIMER0_COMP1",
    "EVENT_I2C_TRANSACTION_DONE",
    "EVENT_I2C_TRANSACTION_ERROR",
    "EVENT_BT_CONNECTION_LOST"
};

//! getEventString()
//! @brief Returns the string representation of the
//! input schedulerEvents_e by indexing into eventString
//!
//! @param ev
//! @returns string representation of ev if valid event
static inline const char *getEventString( schedulerEvents_e ev )
{
    if( ev < NUMBER_OF_EVENTS )
    {
        return eventStrings[ ev ];
    }
    else
    {
        return "";
    }
}

//! Enum defining possible states that scheduler can be in
typedef enum
{
    STATE_SENSOR_OFF,
    STATE_WAIT_FOR_POWERUP,
    STATE_WAIT_FOR_I2C_WRITE,
    STATE_WAIT_FOR_I2C_READ,
    NUMBER_OF_STATES
} schedulerStates_e;

//! String representations for states
static const char *stateStrings[] = {
    "STATE_SENSOR_OFF",
    "STATE_WAIT_FOR_POWERUP",
    "STATE_WAIT_FOR_I2C_WRITE",
    "STATE_WAIT_FOR_I2C_READ"
};

//! getStateString()
//! @brief Returns the string representation of the
//! input schedulerStates_e by indexing into stateStrings
//!
//! @param st
//! @returns string representation of st if valid state
static inline const char *getStateString( schedulerStates_e st )
{
    if( st < NUMBER_OF_STATES )
    {
        return stateStrings[ st ];
    }
    else
    {
        return "";
    }
}

bool schedulerMain( struct gecko_cmd_packet *evt );

static inline bool handleSchedulerEvent( struct gecko_cmd_packet *evt )
{
#if DEVICE_IS_BLE_SERVER == 1
    return schedulerMain( evt );
#else
    return true;
#endif
}

//! schedulerSetEventMeasureTemperature()
//! @brief Trigger a Bluetooth connection lost event for our state machine
//! Call CORE_ATOMIC_IRQ_DISABLE() or CORE_ATOMIC_IRQ_ENABLE() inside here
//!
//! @param void
//! @returns void
static inline void schedulerSetEventConnectionLost()
{
    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_CRITICAL();
    gecko_external_signal( EVENT_BT_CONNECTION_LOST );
    CORE_EXIT_CRITICAL();
    return;
}

//! schedulerSetEventMeasureTemperature()
//! @brief Set event to EVENT_MEASURE_TEMPERATURE, only if there is no pending event
//! Always called from an interrupt context, so don't want to call
//! CORE_ATOMIC_IRQ_DISABLE() or CORE_ATOMIC_IRQ_ENABLE() inside here
//!
//! @param void
//! @returns void
static inline void schedulerSetEventMeasureTemperature()
{
    gecko_external_signal( EVENT_MEASURE_TEMPERATURE );
    return;
}

//! schedulerSetEventTimerDone()
//! @brief Set event to EVENT_LETIMER0_COMP1, only if there is no pending event
//! Always called from an interrupt context, so don't want to call
//! CORE_ATOMIC_IRQ_DISABLE() or CORE_ATOMIC_IRQ_ENABLE() inside here
//!
//! @param void
//! @returns void
static inline void schedulerSetEventTimerDone()
{
    gecko_external_signal( EVENT_LETIMER0_COMP1 );
    return;
}

//! schedulerSetEventTransactionDone()
//! @brief Set event to EVENT_I2C_TRANSACTION_DONE, only if there is no pending event
//! Always called from an interrupt context, so don't want to call
//! CORE_ATOMIC_IRQ_DISABLE() or CORE_ATOMIC_IRQ_ENABLE() inside here
//!
//! @param void
//! @returns void
static inline void schedulerSetEventTransactionDone()
{
    gecko_external_signal( EVENT_I2C_TRANSACTION_DONE );
    return;
}

//! schedulerSetEventTransactionError()
//! @brief Set event to EVENT_I2C_TRANSACTION_ERROR, only if there is no pending event
//! Always called from an interrupt context, so don't want to call
//! CORE_ATOMIC_IRQ_DISABLE() or CORE_ATOMIC_IRQ_ENABLE() inside here
//!
//! @param void
//! @returns void
static inline void schedulerSetEventTransactionError()
{
    gecko_external_signal( EVENT_I2C_TRANSACTION_ERROR );
    return;
}

#endif // __SCHEDULER_H___
