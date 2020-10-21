//!
//! @file irq.c
//! @brief IRQ Handler implementations for LETIMER0 and I2C0
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
//! @resources Utilized Silicon Labs' EMLIB peripheral libraries to implement functionality @n
//!            em_core.h - for CORE_ATOMIC_* macros @n
//!            em_letimer.h - for LETIMER0 interface macros and functions @n
//!            em_i2c.h - for I2C0 interface macros and functions
//!
//! @copyright All rights reserved. Distribution allowed only for the use of assignment grading. Use of code excerpts allowed at the discretion of author. Contact for permission.
//!

#include "irq.h"

#include "main.h"
#include "scheduler.h"
#include "timers.h"

#include "em_core.h"
#include "em_letimer.h"
#include "em_i2c.h"

//! LETIMER0_IRQHandler()
//! @brief Handles interrupts from LETIMER0. If the interrupt
//! is an UF interrupt, then sets the EVENT_MEASURE_TEMPERATURE
//! event for the scheduler and increment our coarse runtime value.
//! Otherwise, if the event is a COMP1 interrupt, then sets the
//! EVENT_LETIMER0_COMP1 event for the scheduler. Only for this
//! interrupt, we also disable future COMP1 interrupts after we
//! have cleared it, since the amount we had to wait has passed
//!
//! @param void
//! @returns void
void LETIMER0_IRQHandler()
{
    CORE_ATOMIC_IRQ_DISABLE();

    int flags = 0;
    flags = LETIMER_IntGet( LETIMER0 );
    if( flags & LETIMER_IF_UF )
    {
        schedulerSetEventMeasureTemperature();
        timerUnderflowHandler();
        LETIMER_IntClear( LETIMER0, LETIMER_IFC_UF );
    }
    else if( flags & LETIMER_IF_COMP1 )
    {
        schedulerSetEventTimerDone();
        LETIMER_IntClear( LETIMER0, LETIMER_IFC_COMP1 );
        LETIMER_IntDisable( LETIMER0, LETIMER_IEN_COMP1 );
    }

    CORE_ATOMIC_IRQ_ENABLE();
}

//! I2C0_IRQHandler()
//! @brief Handles interrupts from I2C0 by calling @ref I2C_Transfer()
//! which returns the status of the on-going transfer. If the returned
//! status is i2cTransferDone, then sets the EVENT_TRANSACTION_DONE event
//! Otherwise, if the status is not equal i2cTransferInProgress, then
//! sets the EVENT_TRANSACTION_ERROR event for the scheduler
//!
//! @param void
//! @returns void
void I2C0_IRQHandler()
{
    CORE_ATOMIC_IRQ_DISABLE();

    I2C_TransferReturn_TypeDef ret = I2C_Transfer( I2C0 );

    if( ret == i2cTransferDone )
    {
        schedulerSetEventTransactionDone();
    }
    else if( ret != i2cTransferInProgress )
    {
        schedulerSetEventTransactionError();
    }

    CORE_ATOMIC_IRQ_ENABLE();
}

