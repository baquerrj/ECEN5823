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

//! Constants for time conversions
static const uint16_t USEC_PER_MSEC = 1000;
static const uint16_t MSEC_PER_SEC = 1000;

//! Constant specifying LETIMER0 COMP0 interrupt period in milliseconds.
//! Changing this value changes the period, e.g. setting it to 2250
//! would set the period to 2.25 seconds
static const uint16_t TIMER_PERIOD_MS = 3000;

//! Uncomment once of the following lines to define the operating energy mode
//#define SLEEP_MODE sleepEM0
//#define SLEEP_MODE sleepEM1
//#define SLEEP_MODE sleepEM2
#define SLEEP_MODE sleepEM3

//! Comment out the following line to wait for interrupts in while-loop
#define ENABLE_SLEEPING
#define LOWEST_ENERGY_MODE  (SLEEP_MODE)


#endif // __MAIN_H___
