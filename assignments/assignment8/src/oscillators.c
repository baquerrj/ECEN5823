//!
//! @file oscillators.c
//! @brief Implementation of Clock Initialization function
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
//!            em_cmu.h - for CMU_* macros and functions
//!
//! @copyright All rights reserved. Distribution allowed only for the use of assignment grading. Use of code excerpts allowed at the discretion of author. Contact for permission.
//!

#include "oscillators.h"

#include "main.h"
#include "log.h"
#include "em_cmu.h"

//! Globally accessible clock frequency in Hz
uint32_t clockFrequencyHz;

//! oscillatorsInit()
//! @brief Set Clock for clock tree LFA depending on the
//! sleep mode @ref SLEEP_MODE selected. Also enable
//! clocks for peripherals
//!
//! @param void
//! @returns void
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
        // Select LFXO for clock tree LFA
        CMU_ClockSelectSet( cmuClock_LFA, cmuSelect_LFXO );
    }
    else if( SLEEP_MODE == sleepEM2 || SLEEP_MODE == sleepEM3 )
    {
        // Enable ULFRCO
        CMU_OscillatorEnable( cmuOsc_ULFRCO, true, true );
        // Select ULFRCO for clock tree LFA
        CMU_ClockSelectSet( cmuClock_LFA, cmuSelect_ULFRCO );
    }

    // Enable High-Frequency Peripheral Clock
    CMU_ClockEnable( cmuClock_HFPER, true );
    // Enable I2C0 Clock
    CMU_ClockEnable( cmuClock_I2C0, true );
    // Enable LFA Clock
    CMU_ClockEnable( cmuClock_LFA, true );
    // Enable LETIMER0 Clock
    CMU_ClockEnable( cmuClock_LETIMER0, true );

    clockFrequencyHz = CMU_ClockFreqGet( cmuClock_LFA );

    LOG_DEBUG( "exiting" );
    return;
}
