/*
 * @file irq.c
 *
 *  Created on: Sep 5, 2020
 *      Author: Roberto Baquerizo
 */

#include "irq.h"

#include "main.h"
#include "gpio.h"

#include "em_core.h"
#include "em_letimer.h"

void LETIMER0_IRQHandler()
{
    CORE_ATOMIC_IRQ_DISABLE();

    int flags = 0;
    flags = LETIMER_IntGet( LETIMER0 );
    if( flags & LETIMER_IF_COMP1 )
    {
        gpioLed0SetOn();
        LETIMER_IntClear( LETIMER0, LETIMER_IFC_COMP1 );
    }
    else if( flags & LETIMER_IF_COMP0 )
    {
        gpioLed0SetOff();
        LETIMER_IntClear( LETIMER0, LETIMER_IFC_COMP0 );
    }

    CORE_ATOMIC_IRQ_ENABLE();
}
