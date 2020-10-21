/*
 * @file scheduler.h
 *
 *  Created on: Sep 14, 2020
 *      Author: Roberto Baquerizo
 */

#ifndef __SCHEDULER_H___
#define __SCHEDULER_H___

#include <stdint.h>
#include <stdbool.h>

//! Value for idle event
#define EVENT_IDLE                  (0)
//! Value for take a temperature measurement event
#define EVENT_MEASURE_TEMPERATURE   (1UL << 1)

//! Used for signaling events
static volatile uint8_t event;

void schedulerInit();

bool schedulerMain( uint8_t _event );

uint8_t schedulerGetEvent();

void schedulerSetEventMeasureTemperature();

void schedulerClearEvent();

//! eventsPresent()
//! @brief Reports whether there any events to process
//!
//! @param void
//! @returns true if there are events to process, false otherwise
static inline bool eventsPresent()
{
    return ( (event > EVENT_IDLE) ? true : false );
}

#endif // __SCHEDULER_H___
