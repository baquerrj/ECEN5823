//!
//! @file timers.h
//! @brief Functions definitions for LETIMER0 interface
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

#ifndef __TIMERS_H___
#define __TIMERS_H___

#include <stdint.h>

void timerInit();

void timerWaitUs( uint32_t waitUs );

uint32_t timerGetRunTimeMilliseconds();

void timerUnderflowHandler();

#endif // __TIMERS_H___
