//!
//! @file timers.c
//! @brief Implements configuration and management of LETIMER0 functionality
//! @version 0.1
//! 
//! @date 2020-09-24
//! @author Roberto Baquerizo (roba8460@colorado.edu)
//! 
//! @institution University of Colorado Boulder (UCB)
//! @course ECEN 5823-001: IoT Embedded Firmware (Fall 2020)
//! @instructor David Sluiter
//! 
//! @assignment ecen5823-assignment4-baquerrj
//! 
//! @resources Utilized Silicon Labs' EMLIB peripheral libraries to implement functionality
//! 
//! @copyright All rights reserved. Distribution allowed only for the use of assignment grading. Use of code excerpts allowed at the discretion of author. Contact for permission.
//!

#include "timers.h"

#include "log.h"
#include "main.h"

#include "stdint.h"
#include "math.h"

#include "em_core.h"

#include "em_cmu.h"
#include "em_letimer.h"

//! Globally accessible clock frequency in Hz (initialized in oscillatorsInit()
extern uint32_t clockFrequencyHz;

//! Coarse Runtime in milliseconds, i.e. increments by 1000 ms on each underflow interrupt
static uint32_t coarseRuntimeMs;

//! Fine Runtime in milliseconds, i.e. time into the current execution period
static uint32_t fineRuntimeMs;

//! Number of times that the underflow interrupt has occurred
static uint32_t underflows;

//! LETIMER0 ticks corresponding to our defined TIMER_PERIOD_MS
static uint16_t periodTicks;

//! Data structure used to initialized LETIMER0 using LETIMER_Init
static const LETIMER_Init_TypeDef timerConfiguration =
{
    .enable = false,                //! Start counting when initialization completes.
    .debugRun = false,              //! Counter will not keep running during debug halt.
    .comp0Top = true,               //! Load COMP0 register into CNT when counter underflows.
    .bufTop = false,                //! Don't load COMP1 into COMP0 when REP0 reaches 0.
    .out0Pol = 0,                   //! Idle value for output 0.
    .out1Pol = 0,                   //! Idle value for output 1.
    .ufoa0 = letimerUFOANone,       //! Underflow output 0 action.
    .ufoa1 = letimerUFOANone,       //! Underflow output 1 action.
    .repMode = letimerRepeatFree,   //! Repeat mode.
    .topValue = 0                   //! Top value. Counter wraps when top value matches counter value is reached.
};

//! calculateAndLoadCompValues()
//! @brief Calculates and loads the number of ticks into COMP0
//! for the requested period
//!
//! @param void
//! @returns void
static void calculateAndLoadCompValues()
{
    double ticks = 0;
    periodTicks = 0;

    ticks = ( clockFrequencyHz * TIMER_PERIOD_MS ) / MSEC_PER_SEC;

    if( ticks > UINT16_MAX )
    {
        int i = 0;
        int prescaler = 0;
        i = ticks / UINT16_MAX;
        prescaler = pow( 2, i );

        periodTicks = ticks / prescaler;

        CMU_ClockDivSet( cmuClock_LETIMER0, prescaler );
    }
    else if( ticks <= UINT16_MAX )
    {
        periodTicks = ( uint16_t ) ticks;
    }

    //! COMP0 is set to the number of ticks
    //! corresponding to TIMER_PERIOD_MS
    LETIMER_CompareSet( LETIMER0, 0, periodTicks );
    return;
}

//! timerInit()
//! @brief Initialize LETIMER0 with timerConfiguration and loads
//! the LETIMER0 COMP0 value
//!
//! @param void
//! @returns void
void timerInit()
{
    //! Clear any pending LETIMER0 COMP1 and Underflow interrupts
    LETIMER_IntClear( LETIMER0, LETIMER_IFC_UF | LETIMER_IFC_COMP1 );
    LETIMER_Init( LETIMER0, &timerConfiguration );

    while( LETIMER0->SYNCBUSY != 0 ) {}

    LETIMER0->IFC &= 0ULL;

    calculateAndLoadCompValues();

    coarseRuntimeMs = 0;
    fineRuntimeMs = 0;
    underflows = 0;
    // Enable LETIMER0 UF Interrupts
    LETIMER_IntEnable( LETIMER0, LETIMER_IEN_UF );
    LOG_DEBUG( "exiting" );
    return;
}

//! timerWaitUs()
//! @brief Blocks execution for the requested number of microseconds.
//! Performs range checking to handle underflow cases or if
//! the timer cannot support the requested wait time.
//!
//! @param waitUs Wait time in microseconds
//! @returns
void timerWaitUs( uint32_t waitUs )
{
    // Disable interrupts while we calculate COMP1 value
    CORE_ATOMIC_IRQ_DISABLE();
    uint32_t startTicks = LETIMER_CounterGet( LETIMER0 );

    if( waitUs > ( TIMER_PERIOD_MS * USEC_PER_MSEC ) )
    {
        // cap the wait time to maximum time timer can wait
        // which is the value we loaded into the COMP0 register
        // which is the period in ticks
        waitUs = TIMER_PERIOD_MS * USEC_PER_MSEC;
        LOG_WARN( "Requested wait time is longer than what is supported. Capped wait time to %lu usecs", waitUs );
    }
    uint16_t waitMs = waitUs / USEC_PER_MSEC;
    uint16_t waitTicks = ( clockFrequencyHz * waitMs ) / MSEC_PER_SEC;
    double stopTicks = 0;

    if( startTicks > waitTicks )
    {
        stopTicks = startTicks - waitTicks;
    }
    else
    {
        stopTicks = periodTicks - ( waitTicks - startTicks );
    }

    // Clear any pending COMP1 interrupt, load stopTicks into COMP1 register
    // and enable the COMP1 interrupt
    LETIMER_IntClear( LETIMER0, LETIMER_IFC_COMP1 );
    LETIMER_CompareSet( LETIMER0, 1, ( uint16_t ) stopTicks );
    LETIMER_IntEnable( LETIMER0, LETIMER_IEN_COMP1 );

    // Re-enable interrupts now that we have loaded the COMP1 value
    // and enabled the COMP1 interrupt
    CORE_ATOMIC_IRQ_ENABLE();
    return;
}

//! timerGetRunTimeMilliseconds()
//! @brief Returns current runtime in milliseconds using LETIMER0 as a
//! reference for the time returned
//!
//! @param void
//! @returns uint32_t runtime in milliseconds
uint32_t timerGetRunTimeMilliseconds()
{
    uint32_t currentTicks = LETIMER_CounterGet( LETIMER0 );

    if( currentTicks != 0 )
    {
        fineRuntimeMs = periodTicks - currentTicks;
    }

    return (coarseRuntimeMs + fineRuntimeMs);
}

//! timerUnderflowHandler()
//! @brief Handle the change to runtime when underflow occurs
//!
//! @param void
//! @returns void
void timerUnderflowHandler()
{
    underflows++;
    coarseRuntimeMs = underflows * TIMER_PERIOD_MS;
}

