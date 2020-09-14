/*
 * @file main.h
 *
 *  Created on: Sep 5, 2020
 *      Author: Roberto Baquerizo
 */

#ifndef __MAIN_H___
#define __MAIN_H___

#include "stdint.h"
#include "sleep.h"

static const uint16_t MSEC_PER_SEC = 1000;

static const uint16_t LED_ON_TIME_MS = 175;
//static const uint16_t LED_ON_TIME_MS = 50;
//static const uint16_t LED_ON_TIME_MS = 1000;

static const uint16_t TIMER_PERIOD_MS = 2250;
//static const uint16_t TIMER_PERIOD_MS = 500;
//static const uint16_t TIMER_PERIOD_MS = 7000;


// Uncomment once of the following lines to define the
// energy mode to operate in
//#define SLEEP_MODE sleepEM0
//#define SLEEP_MODE sleepEM1
#define SLEEP_MODE sleepEM2
//#define SLEEP_MODE sleepEM3

// Comment out the following line to wait for interrupts in while-loop
#define ENABLE_SLEEPING
#define LOWEST_ENERGY_MODE  (SLEEP_MODE)


#endif // __MAIN_H___
