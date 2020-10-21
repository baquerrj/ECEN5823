/*
 * @file timers.h
 *
 *  Created on: Sep 3, 2020
 *      Author: Roberto Baquerizo
 */

#ifndef __TIMERS_H___
#define __TIMERS_H___

#include <stdint.h>

void timerInit();

void timerWaitUs( uint32_t waitUs );

#endif // __TIMERS_H___
