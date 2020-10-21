//!
//! @file main.c
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
//! @assignment ecen5823-assignment7-baquerrj
//!
//! @resources Utilized Silicon Labs' EMLIB peripheral libraries to implement functionality
//!
//! @copyright All rights reserved. Distribution allowed only for the use of assignment grading. Use of code excerpts allowed at the discretion of author. Contact for permission.
//!

#include "main.h"

#include "log.h"
#include "scheduler.h"
#include "gpio.h"
#include "i2c.h"
#include "timers.h"
#include "oscillators.h"
#include "irq.h"
#include "display.h"
#include "ble.h"

#include "gatt_db.h"
#include "gecko_configuration.h"
#include "native_gecko.h"

#include "em_letimer.h"
#include "em_cmu.h"
#include "em_emu.h"


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
    SLEEP_SleepBlockBegin( sleepEM3 );

    //! Initialize Clock Management Unit
    oscillatorsInit();

    //! Initialize LETIMER0
    timerInit();

    //! Enable LETIMER0 so it begins counting
    LETIMER_Enable( LETIMER0, true );

    displayInit();
    displayPrintf( DISPLAY_ROW_NAME, "Server" );
    NVIC_EnableIRQ( LETIMER0_IRQn );

    //! Infinite while-loop
    while( 1 )
    {
        struct gecko_cmd_packet *evt;

        if( !gecko_event_pending() )
        {
            logFlush();
        }
        evt = gecko_wait_event();

        // We have a pending event, so get the event from scheduler and
        // process the returned event
        bool eventHandled = true;
        eventHandled = handleBleEvent( evt );

        if( !eventHandled )
        {
            // If eventHandled == false, handleBleEvent() could not handle the
            // returned event, so maybe it was one of the external signals
            eventHandled = handleSchedulerEvent( evt );
            if( !eventHandled )
            {
                LOG_ERROR( "Encountered an error while handling event" );
            }
        }
    }
}
