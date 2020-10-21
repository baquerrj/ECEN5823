/*
 * @file main.c
 *
 *  Created on: Sep 3, 2020
 *      Author: Roberto Baquerizo
 *
 * @resources Utilized Silicon Labs' EMLIB peripheral libraries to implement functionality.
 *
 */

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
    bool sleepingConfigured = false;
#ifdef ENABLE_SLEEPING
    if( SLEEP_MODE != sleepEM0 )
    {
        SLEEP_Init_t sleepConfig = {0};
        SLEEP_InitEx( &sleepConfig );
        SLEEP_SleepBlockBegin( LOWEST_ENERGY_MODE+1 );  // block entry to next lowest energy level
        sleepingConfigured = true;
    }
#endif
    //! Initialize Clock Management Unit
    oscillatorsInit();

    //! Initialize LETIMER0
    timerInit();

    //! Initialize scheduler
    schedulerInit();

    //! Initialize I2C
    i2cInit();

    //! Clear any pending LETIMER0 COMP0 and Underflow interrupts
    LETIMER_IntClear( LETIMER0, LETIMER_IFC_COMP0 | LETIMER_IFC_UF );

    //! Enable LETIMER0 COMP0 Interrupts
    LETIMER_IntEnable( LETIMER0, LETIMER_IEN_COMP0 );
    NVIC_EnableIRQ( LETIMER0_IRQn );

    //! Enable LETIMER0 so it begins counting
    LETIMER_Enable( LETIMER0, true );

    bool status = true;

    //! Infinite while-loop
    while( 1 )
    {
        uint8_t pendingEvent = EVENT_IDLE;
        // If sleeping, check for pending events. If no events are pending
        // go to sleep
        if( true == sleepingConfigured )
        {
            if( false == eventsPresent() )
            {
                logFlush();
                SLEEP_Sleep();
            }

            // We have a pending event, so get the event from scheduler and
            // process the returned event
            pendingEvent = schedulerGetEvent();
            status = schedulerMain( pendingEvent );
        }
        else
        {
            // If not sleeping, check for pending events. Wait for an interrupt
            // and then check for pending interrupts once we receive an interrupt
            __WFI();
            if( true == eventsPresent() )
            {
                // We have a pending event, so get the event from scheduler and
                // process the returned event
                pendingEvent = schedulerGetEvent();
                status = schedulerMain( pendingEvent );
            }
        }
        // Something went wrong, log it.
        if( status != true )
        {
            LOG_ERROR( "Encountered an error while processing event [%d]", pendingEvent );
        }
    }
}
