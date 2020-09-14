#include "gecko_configuration.h"
#include "gpio.h"
#include "native_gecko.h"

static void delayApproxOneSecond( void )
{
    /**
     * Wait loops are a bad idea in general!  Don't copy this code in future assignments!
     * We'll discuss how to do this a better way in the next assignment.
     */
    volatile int i;
    for( i = 0; i < 3500000; )
    {
        i = i + 1;
    }
}

int appMain( gecko_configuration_t *config )
{
    // Initialize stack
    gecko_init( config );

    // Initialize GPIO
    gpioInit();

    /* Infinite loop */
    while( 1 )
    {
        // 50% duty cycle - 1 sec on & 1 sec off
        // LED 0 and 1 off for 1 sec
        gpioLed1SetOff();
        gpioLed0SetOff();
        delayApproxOneSecond();

        // LED 0 and 1 on for 1 sec
        gpioLed1SetOn();
        gpioLed0SetOn();
        delayApproxOneSecond();
    }
}
