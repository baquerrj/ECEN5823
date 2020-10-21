//!
//! @file main.h
//! @brief 
//! @version 0.1
//! 
//! @date 2020-09-24
//! @author Roberto Baquerizo (roba8460@colorado.edu)
//! 
//! @institution University of Colorado Boulder (UCB)
//! @course ECEN 5823-001: IoT Embedded Firmware (Fall 2020)
//! @instructor David Sluiter
//! 
//! @assignment ecen5823-assignment5-baquerrj
//! 
//! @resources Utilized Silicon Labs' EMLIB peripheral libraries to implement functionality
//! 
//! @copyright All rights reserved. Distribution allowed only for the use of assignment grading. Use of code excerpts allowed at the discretion of author. Contact for permission.
//!

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
// #define SLEEP_MODE sleepEM0
// #define SLEEP_MODE sleepEM1
// #define SLEEP_MODE sleepEM2
#define SLEEP_MODE sleepEM3

//! Comment out the following line to wait for interrupts in while-loop
#define ENABLE_SLEEPING
#define LOWEST_ENERGY_MODE  (SLEEP_MODE)


#endif // __MAIN_H___
