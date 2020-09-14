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


int appMain( gecko_configuration_t *config )
{
    // Initialize stack
    gecko_init( config );

    // Initialize GPIO
    gpioInit();

    // If we want to at least enter EM2 in while-loop
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
    // Initialize Clock Management Unit
    oscillatorsInit();

    // Initialize LETIMER0
    timerInit();

    LETIMER_IntClear( LETIMER0, LETIMER_IFC_COMP0 | LETIMER_IFC_UF );

    // Enable COMP0 and COMP1 Interrupts
    LETIMER_IntEnable( LETIMER0, LETIMER_IEN_COMP0 );
    LETIMER_IntEnable( LETIMER0, LETIMER_IEN_COMP1 );

    NVIC_EnableIRQ( LETIMER0_IRQn );

    LETIMER_Enable( LETIMER0, true );

    /* Infinite loop */
    while( 1 )
    {
        if( true == sleepingConfigured )
        {
            SLEEP_Sleep();
        }
        else
        {
            __WFI();
        }
    }
}
