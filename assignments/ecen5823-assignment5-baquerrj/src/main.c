//!
//! @file main.c
//! @brief 
//! @version 0.1
//! 
//! @date 2020-09-24
//! @author Roberto Baquerizo (roba8460@colorado.edu)
//! 
//! @institution University of Colorado Boulder (UCB)
//! @course ECEN 5823-001: IoT Embedded Firmware (Fall 2020)
//! @instructor David Sluiter
//! 
//! @assignment ecen5823-assignment5-baquerrj
//! 
//! @resources Utilized Silicon Labs' EMLIB peripheral libraries to implement functionality
//! 
//! @copyright All rights reserved. Distribution allowed only for the use of assignment grading. Use of code excerpts allowed at the discretion of author. Contact for permission.
//!

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

#include "log.h"
#include "scheduler.h"
#include "i2c.h"
#include "ble.h"
#include "gatt_db.h"

// #define DEBUG_BT
#ifdef DEBUG_BT
static bool bleConnected = false;
#endif

int appMain( gecko_configuration_t *config )
{
    //! Initialize logging
    logInit();

    //! Initialize stack
    gecko_init( config );

    //! Initialize GPIO
    gpioInit();

    //! If we want to at least enter EM2 in while-loop
    SLEEP_Init_t sleepConfig = {0};
    SLEEP_InitEx( &sleepConfig );
    SLEEP_SleepBlockBegin( sleepEM3 );

    //! Initialize Clock Management Unit
    oscillatorsInit();

    //! Initialize LETIMER0
    timerInit();

    //! Initialize scheduler
    schedulerInit();

    //! Enable LETIMER0 so it begins counting
    LETIMER_Enable( LETIMER0, true );

    NVIC_EnableIRQ( LETIMER0_IRQn );

    //! Infinite while-loop
    while( 1 )
    {
        struct gecko_cmd_packet *evt;

        if( !gecko_event_pending() )
        {
            logFlush();
        }
        evt = gecko_wait_event();

        // We have a pending event, so get the event from scheduler and
        // process the returned event
        bool eventHandled = true;
#ifndef DEBUG_BT
        eventHandled = handleBleEvent( evt );
#else
        switch( BGLIB_MSG_ID( evt->header ) )
        {
            case gecko_evt_system_boot_id:
            {
                LOG_INFO( "System boot" );
                gecko_cmd_le_gap_set_advertise_timing( 0, 160, 160, 0, 0 );
                gecko_cmd_le_gap_start_advertising( 0, le_gap_general_discoverable, le_gap_connectable_scannable );
                break;
            }
            case gecko_evt_le_connection_opened_id:
            {
                LOG_INFO( "Connection opened" );
                bleConnected = true;
                break;
            }
            case gecko_evt_gatt_server_characteristic_status_id:
            {
                if( ( evt->data.evt_gatt_server_characteristic_status.characteristic == gattdb_temperature_measurement )
                    && ( evt->data.evt_gatt_server_characteristic_status.status_flags == gatt_server_client_config ) )
                {
                    if( evt->data.evt_gatt_server_characteristic_status.client_config_flags == gatt_indication )
                    {
                        LOG_DEBUG( "GATT Indication Flag received" );
                    }
                    else if( evt->data.evt_gatt_server_characteristic_status.client_config_flags == gatt_disable )
                    {
                        LOG_DEBUG( "GATT Disable Flag received" );
                        bleConnected = false;
                    }
                }
                break;
            }
            case gecko_evt_le_connection_closed_id:
            {
                bleConnected = false;
                gecko_cmd_le_gap_start_advertising( 0, le_gap_general_discoverable, le_gap_connectable_scannable );
                break;
            }
            case gecko_evt_system_external_signal_id:
            {
                eventHandled = false;
                break;
            }
            default:
            {
                break;
            }
        }
#endif

#ifdef DEBUG_BT
        if( !eventHandled && bleConnected )
#else
        if( !eventHandled )
#endif
        {
            // If eventHandled == false, handleBleEvent() could not handle the
            // returned event, so maybe it was one of the external signals
            eventHandled = schedulerMain( evt );
            if( !eventHandled )
            {
                LOG_ERROR( "Encountered an error while handling event" );
            }
        }
    }
}
