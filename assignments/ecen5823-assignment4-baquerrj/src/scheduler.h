//!
//! @file scheduler.h
//! @brief Functions and type definitions for our sheduler
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

#ifndef __SCHEDULER_H___
#define __SCHEDULER_H___

#include <stdint.h>
#include <stdbool.h>

//! Enum defining possible events that scheduler can process
typedef enum
{
    EVENT_IDLE,
    EVENT_MEASURE_TEMPERATURE,
    EVENT_LETIMER0_COMP1,
    EVENT_I2C_TRANSACTION_DONE,
    EVENT_I2C_TRANSACTION_ERROR,
    NUMBER_OF_EVENTS
} events_e;

//! String representations for events
static const char* eventStrings[] = {
    "EVENT_IDLE",
    "EVENT_MEASURE_TEMPERATURE",
    "EVENT_LETIMER0_COMP1",
    "EVENT_I2C_TRANSACTION_DONE",
    "EVENT_I2C_TRANSACTION_ERROR"
};

//! getEventString()
//! @brief Returns the string representation of the 
//! input events_e by indexing into eventString
//!
//! @param ev
//! @returns string representation of ev if valid event
static inline const char* getEventString( events_e ev )
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
} states_e;

//! String representations for states
static const char* stateStrings[] = {
    "STATE_SENSOR_OFF",
    "STATE_WAIT_FOR_POWERUP",
    "STATE_WAIT_FOR_I2C_WRITE",
    "STATE_WAIT_FOR_I2C_READ"
};

//! getStateString()
//! @brief Returns the string representation of the 
//! input states_e by indexing into stateStrings
//!
//! @param st
//! @returns string representation of st if valid state
static inline const char* getStateString( states_e st )
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

void schedulerInit();

bool schedulerMain( events_e _event );

uint8_t schedulerGetEvent();

void schedulerSetEventMeasureTemperature();

void schedulerSetEventTimerDone();

void schedulerSetEventTransactionDone();

void schedulerSetEventTransactionError();

void schedulerClearEvent();

bool eventsPresent();

#endif // __SCHEDULER_H___
