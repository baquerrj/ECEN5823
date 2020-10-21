/*
 * @file timers.c
 *
 *  Created on: Sep 3, 2020
 *      Author: Roberto Baquerizo
 */

#include "timers.h"

#include "log.h"
#include "main.h"
#include "gpio.h"

#include "stdint.h"
#include "math.h"

#include "em_cmu.h"
#include "em_letimer.h"

//! Globally accessible clock frequency in Hz (initialized in oscillatorsInit()
extern uint32_t clockFrequencyHz;

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
    uint16_t comp0Value = 0;

    ticks = ( clockFrequencyHz * TIMER_PERIOD_MS ) / MSEC_PER_SEC;

    if( ticks > UINT16_MAX )
    {
        int i = 0;
        int prescaler = 0;
        i = ticks / UINT16_MAX;
        prescaler = pow( 2, i );

        comp0Value = ticks / prescaler;

        CMU_ClockDivSet( cmuClock_LETIMER0, prescaler );
    }
    else if( ticks <= UINT16_MAX )
    {
        comp0Value = ( uint16_t ) ticks;
    }

    //! COMP0 is set to the number of ticks
    //! corresponding to TIMER_PERIOD_MS
    LETIMER_CompareSet( LETIMER0, 0, comp0Value );
}

//! timerInit()
//! @brief Initialize LETIMER0 with timerConfiguration and loads
//! the LETIMER0 COMP0 value
//!
//! @param void
//! @returns void
void timerInit()
{
    LETIMER_IntClear( LETIMER0, LETIMER_IFC_COMP0 );

    LETIMER_Init( LETIMER0, &timerConfiguration );

    while( LETIMER0->SYNCBUSY != 0 ) {}

    LETIMER0->IFC &= 0ULL;

    calculateAndLoadCompValues();
    LOG_DEBUG( "exiting" );
    return;
}

//! timerWaitUs()
//! @brief Blocks execution for the requested number of microseconds.
//! Performs range checking to handle underflow cases or if
//! the timer cannot support the requested wait time.
//!
//! @param[in] waitUs Wait time in microseconds
//! @returns
void timerWaitUs( uint32_t waitUs )
{
    uint32_t startTicks = LETIMER_CounterGet( LETIMER0 );

    if( (waitUs / USEC_PER_MSEC) > UINT16_MAX )
    {
        // cap the wait time to maximum time timer can wait
        // which is the maximum value that can be stored in a uint16_t
        waitUs = UINT16_MAX;
        LOG_WARN( "Requested wait time is longer than what is supported. Capped wait time to %lu usecs", waitUs );
    }
    uint16_t waitMs = waitUs / USEC_PER_MSEC;
    uint16_t waitTicks = ( clockFrequencyHz * waitMs ) / MSEC_PER_SEC;
    double stopTicks = startTicks - waitTicks;
    if( stopTicks < 0 )
    {
        // handle underflow of ticks
        uint16_t tmp = stopTicks * -1;
        stopTicks = UINT16_MAX - tmp;
    }
    while( true )
    {
        if( (uint16_t) stopTicks >= LETIMER_CounterGet( LETIMER0 ) )
        {
            break;
        }
    }

    LOG_DEBUG( "exiting" );
    return;
}
