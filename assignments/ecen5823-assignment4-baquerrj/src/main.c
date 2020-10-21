//!
//! @file main.c
//! @brief 
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

#include "main.h"

#include "gecko_configuration.h"
#include "gpio.h"
#include "native_gecko.h"

#include "timers.h"
#include "oscillators.h"
#include "irq.h"

#include "em_letimer.h"
#include "em_cmu.h"
#include "em_emu.h"

#include "log.h"
#include "scheduler.h"
#include "i2c.h"

int appMain( gecko_configuration_t *config )
{
    //! Initialize logging
    logInit();

    //! Initialize stack
    gecko_init( config );

    //! Initialize GPIO
    gpioInit();

    //! If we want to at least enter EM2 in while-loop
    SLEEP_Init_t sleepConfig = {0};
    SLEEP_InitEx( &sleepConfig );

    //! Initialize Clock Management Unit
    oscillatorsInit();

    //! Initialize LETIMER0
    timerInit();

    //! Initialize scheduler
    schedulerInit();

    //! Enable LETIMER0 so it begins counting
    LETIMER_Enable( LETIMER0, true );

    NVIC_EnableIRQ( LETIMER0_IRQn );

    bool status = true;

    //! Infinite while-loop
    while( 1 )
    {
        events_e pendingEvent = EVENT_IDLE;
        if( false == eventsPresent() )
        {
            logFlush();
            SLEEP_Sleep();
        }

        // We have a pending event, so get the event from scheduler and
        // process the returned event
        pendingEvent = schedulerGetEvent();
        status = schedulerMain( pendingEvent );
        if( status != true )
        {
            LOG_ERROR( "Encountered an error while handling event [%d]", pendingEvent );
        }
    }
}
