/*
 * @file timers.c
 *
 *  Created on: Sep 3, 2020
 *      Author: Roberto Baquerizo
 */

#include "timers.h"

#include "main.h"
#include "stdint.h"
#include "math.h"

#include "em_cmu.h"

#include "em_letimer.h"

extern uint32_t clockFrequencyHz;

static const LETIMER_Init_TypeDef timerConfiguration =
{
    .enable = false,   // Start counting when initialization completes.
    .debugRun = false, // Counter shall keep running during debug halt.
#if defined(LETIMER_CTRL_RTCC0TEN)
    .rtcComp0Enable = false, // Start counting on RTC COMP0 match.
    .rtcComp1Enable = false, // Start counting on RTC COMP1 match.
#endif
    .comp0Top = true,             // Load COMP0 register into CNT when counter underflows.
    .bufTop = false,              // Don't load COMP1 into COMP0 when REP0 reaches 0.
    .out0Pol = 0,                 // Idle value for output 0.
    .out1Pol = 0,                 // Idle value for output 1.
    .ufoa0 = letimerUFOANone,    // Underflow output 0 action.
    .ufoa1 = letimerUFOANone,    // Underflow output 1 action.
    .repMode = letimerRepeatFree, // Repeat mode.
    .topValue = 0                 // Top value. Counter wraps when top value matches counter value is reached.
};

void calculateAndLoadCompValues()
{
    double ticks = 0;
    uint16_t comp0Value = 0;
    uint16_t comp1Value = 0;

    ticks = ( clockFrequencyHz * TIMER_PERIOD_MS ) / MSEC_PER_SEC;

    if( ticks > UINT16_MAX )
    {
        int i = 0;
        int prescaler = 0;
        i = ticks / UINT16_MAX;
        prescaler = pow( 2, i );
        int actualFrequency = clockFrequencyHz / prescaler;

        comp0Value = ticks / prescaler;
        comp1Value = ( actualFrequency * LED_ON_TIME_MS ) / MSEC_PER_SEC;

        CMU_ClockDivSet( cmuClock_LETIMER0, prescaler );
    }
    else if( ticks <= UINT16_MAX )
    {
        comp0Value = ( uint16_t ) ticks;
        comp1Value = ( clockFrequencyHz * LED_ON_TIME_MS ) / MSEC_PER_SEC;
    }

    // COMP0 is used to turn on the LED, defining the timer period
    LETIMER_CompareSet( LETIMER0, 0, comp0Value );
    // COMP1 is used to turn off the LED, defining the LED on time
    LETIMER_CompareSet( LETIMER0, 1, comp1Value );
}

void timerInit()
{
    LETIMER_IntClear( LETIMER0, LETIMER_IFC_COMP0 );

    LETIMER_Init( LETIMER0, &timerConfiguration );

    while( LETIMER0->SYNCBUSY != 0 ) {}

    LETIMER0->IFC &= 0ULL;

    calculateAndLoadCompValues();
    return;
}
