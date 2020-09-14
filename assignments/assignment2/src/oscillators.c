/*
 * @file oscillators.c
 *
 *  Created on: Sep 5, 2020
 *      Author: Roberto Baquerizo
 */

#include "oscillators.h"

#include "main.h"
#include "em_cmu.h"

uint32_t clockFrequencyHz;

void oscillatorsInit()
{
    CMU_ClockEnable( cmuClock_GPIO, true );

    if( SLEEP_MODE > LOWEST_ENERGY_MODE )
    {
        return;
    }

    if( SLEEP_MODE == sleepEM0 || SLEEP_MODE == sleepEM1 )
    {
            // Configure for Low-Frequency Crystal Oscillator
            CMU_OscillatorEnable( cmuOsc_LFXO, true, true );
            CMU_ClockSelectSet( cmuClock_LFA, cmuSelect_LFXO ); // Select LFXO for clock tree LFA
    }
    else if( SLEEP_MODE == sleepEM2 || SLEEP_MODE == sleepEM3 )
    {
        CMU_OscillatorEnable( cmuOsc_ULFRCO, true, true );      // Enable ULFRCO
        CMU_ClockSelectSet( cmuClock_LFA, cmuSelect_ULFRCO );   // Select ULFRCO for clock tree LFA
    }

    CMU_ClockEnable( cmuClock_LFA, true );
    CMU_ClockEnable( cmuClock_LETIMER0, true );

    clockFrequencyHz = CMU_ClockFreqGet( cmuClock_LFA );

    return;
}
