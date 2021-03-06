//!
//! @file log.h
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
//! @assignment ecen5823-assignment6-baquerrj
//! 
//! @resources Originally written by Dan Walkes - modified by Roberto Baquerizo
//! 
//! @copyright All rights reserved. Distribution allowed only for the use of assignment grading. Use of code excerpts allowed at the discretion of author. Contact for permission.
//!

#ifndef SRC_LOG_H_
#define SRC_LOG_H_
#include "stdio.h"
#include <inttypes.h>

// Un-comment the following line to enable Debug-level logging  (LOG_DEBUG)
//#define INCLUDE_LOG_DEBUG
// Un-comment the following line to enable reporting temperature
// measurement even when INCLUDE_LOGGING is not defined
//#define LOG_TEMPERATURE_ONLY

/**
 * Instructions for using this module:
 * 1) #include "log.h" in the C file where you'd like to add logging
 * 2) Reference macros LOG_XXX to log at specific thresholds.  Use printf style argument
 *    lists with each macro to support printing variables.
 *    * ERROR for unexpected conditions which should never occur.
 *    * WARN for potential issues which may occur
 *    * INFO for infrequent status updates
 *    * DEBUG for detailed updates, useful when troubleshooting a specific
 *        code path
 *  3) Call logInit() once in the main init routine, before any logging is attempted and after any timing capability for
 *  loggerGetTimestamp() is complete.
 *  For the Blue Gecko platform, logging can be viewed with Tera Term or similar terminal emulator.
 *       Select the "JLink CDC UART Port" and baud rate 115200.
 *  All logging is off by default (compiled out of the build)
 *   * To turn on you should #define INCLUDE_LOGGING 1 in your project files or setup a build configuration for
 *       this purpose.
 *  Even after turning logging on with INCLUDE_LOGGING, debug logging is off by default.
 *   * To turn debug logging on for a specific .c file, #define INCLUDE_LOG_DEBUG 1 at the top of the file
 *       before the #include "log.h" reference.
 *   * To turn on for all files #define INCLUDE_LOG_DEBUG 1 in the project configuration.
 */
#ifndef LOG_ERROR
#define LOG_ERROR(message,...) \
	LOG_DO(message,"Error", ##__VA_ARGS__)
#endif

#ifndef LOG_WARN
#define LOG_WARN(message,...) \
	LOG_DO(message,"Warn ", ##__VA_ARGS__)
#endif

#ifndef LOG_INFO
#define LOG_INFO(message,...) \
	LOG_DO(message,"Info ", ##__VA_ARGS__)
#endif

#ifdef INCLUDE_LOG_DEBUG
#ifndef LOG_DEBUG
#define LOG_DEBUG(message,...) \
	LOG_DO(message,"Debug",##__VA_ARGS__)
#define LOG_DEBUG_CODE(code) code
#endif
#else
#define LOG_DEBUG(message,...)
#define LOG_DEBUG_CODE(code)
#endif

#ifndef LOG_TEMPERATURE
#define LOG_TEMPERATURE(measurement) \
    LOG_TEMP_DO(measurement, "SI7021 Temperature Measurement" )
#endif

#if INCLUDE_LOGGING
#define LOG_DO(message,level, ...) \
	printf( "%5"PRIu32":%s:%s: " message "\n", loggerGetTimestamp(), level, __func__, ##__VA_ARGS__ )
#endif
#if (defined(INCLUDE_LOGGING) || defined(LOG_TEMPERATURE_ONLY))
// Define LOG_TEMP_DO to report the temperature measurement
#define LOG_TEMP_DO(measurement,  temperature) \
        printf( "%5"PRIu32":%s:%s: %3.1f C\n" , loggerGetTimestamp(), temperature, __func__, measurement )
void logInit();
uint32_t loggerGetTimestamp();
void logFlush();
#endif

#if (!defined(INCLUDE_LOGGING) && defined(LOG_TEMPERATURE_ONLY))
// On builds where general logging is not enabled but temperature logging is, only
// remove all general logging related code
#define LOG_DO(message,level, ...)
#else
#if (!defined(INCLUDE_LOGGING) && !defined(LOG_TEMPRERATURE_ONLY))
/**
 * Remove all logging related code on builds where logging is not enabled
 */
#define LOG_DO( message, level, ... )
#define LOG_TEMP_DO( measurement, temperature, ... )

static inline void logInit()
{}

static inline uint32_t loggertGetTimestamp()
{
	return 0;
}

static inline void logFlush()
{}

#endif
#endif

#endif /* SRC_LOG_H_ */
