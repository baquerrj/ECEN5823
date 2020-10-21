/*
 * @file scheduler.c
 *
 *  Created on: Sep 14, 2020
 *      Author: Roberto Baquerizo
 */


#include "scheduler.h"

#include "log.h"
#include "gpio.h"
#include "i2c.h"

#include "em_core.h"

void schedulerInit()
{
    event = EVENT_IDLE;

    if( event != EVENT_IDLE )
    {
        LOG_ERROR( "Could not initialize event to 0!" );
    }

    LOG_DEBUG( "exiting" );
    return;
}

//! @brief Process the input event
//!
//! @paramp _event event to process
//! @returns true if successfully, false if encountered an error
bool schedulerMain( uint8_t _event )
{
    uint8_t tmp = _event;
    bool status = true;
    switch( tmp )
    {
        case EVENT_IDLE:
        {
            break;
        }
        case EVENT_MEASURE_TEMPERATURE:
        {
            LOG_DEBUG( "Registered event [%d]", tmp );
            double data;
            status = i2cGetTemperature( &data );
            LOG_TEMPERATURE( data );
            break;
        }
        default:
        {
            LOG_WARN( "Unknown event detected [%d]", tmp );
            status = false;
            break;
        }
    }
    return status;
}

//! schedulerGetEvent()
//! @brief Returns event containing events to be proccessed and clears it
//! When clearing events, disable interrupts so the operation is atomic
//!
//! @param void
//! @returns event
uint8_t schedulerGetEvent()
{
    uint8_t _event = 0;
    _event = event;
    CORE_ATOMIC_IRQ_DISABLE();
    schedulerClearEvent();
    CORE_ATOMIC_IRQ_ENABLE();

    LOG_DEBUG( "exiting" );
    return _event;
}

//! schedulerSetEventMeasureTemperature()
//! @brief Set the EVENT_MEASURE_TEMPERATURE bit in event
//! Always called from an interrupt context, so don't want to call
//! CORE_ATOMIC_IRQ_DISABLE() or CORE_ATOMIC_IRQ_ENABLE() inside here
//!
//! @param void
//! @returns void
void schedulerSetEventMeasureTemperature()
{
    event |= EVENT_MEASURE_TEMPERATURE;
    LOG_DEBUG( "exiting" );
    return;
}

//! schedulerClearEvent()
//! @brief Clear any pending events by setting event to EVENT_IDLE
//! It is the responsibility of the caller to ensure that
//! interrupts are disabled while we clear events
//!
//! @param void
//! @returns void
void schedulerClearEvent()
{
    event = EVENT_IDLE;
    LOG_DEBUG( "exiting" );
    return;
}

//! schedulerMain()
