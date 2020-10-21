/*
 * @file irq.c
 *
 *  Created on: Sep 5, 2020
 *      Author: Roberto Baquerizo
 */

#include "irq.h"

#include "main.h"
#include "scheduler.h"

#include "em_core.h"
#include "em_letimer.h"

//! LEETIMER0_IRQHandler()
//! @brief Handles interrupts from LETIMER0. If the interrupt
//! is a COMP0 interrupt, then set appropriate event for the scheduler
//!
//! @param void
//! @returns void
void LETIMER0_IRQHandler()
{
    CORE_ATOMIC_IRQ_DISABLE();

    int flags = 0;
    flags = LETIMER_IntGet( LETIMER0 );
    if( flags & LETIMER_IF_COMP0 )
    {
        schedulerSetEventMeasureTemperature();
        LETIMER_IntClear( LETIMER0, LETIMER_IFC_COMP0 );
    }

    CORE_ATOMIC_IRQ_ENABLE();
}
