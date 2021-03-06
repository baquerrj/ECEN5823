/*
 * gpio.c
 *
 *  Created on: Dec 12, 2018
 *      Author: Dan Walkes
 */
#include "gpio.h"

#include "log.h"

//! gpioInit()
//! @brief Initialize GPIOs for LEDs and Si7021 sensor
//!
//! @param void
//! @returns void
void gpioInit()
{
	GPIO_DriveStrengthSet(LED0_port, gpioDriveStrengthWeakAlternateWeak);
    GPIO_PinModeSet( LED0_port, LED0_pin, gpioModePushPull, false );

	GPIO_DriveStrengthSet(LED1_port, gpioDriveStrengthWeakAlternateWeak);
    GPIO_PinModeSet( LED1_port, LED1_pin, gpioModePushPull, false );

    GPIO_PinModeSet( SI7021_PORT, SI7021_PIN, gpioModePushPull, true );
    LOG_DEBUG( "exiting" );
}

//! gpioLed0SetOn()
//! @brief Set GPIO for LED0
//!
//! @param void
//! @returns void
void gpioLed0SetOn()
{
    GPIO_PinOutSet( LED0_port, LED0_pin );
}

//! gpioLed0SetOff()
//! @brief Clears GPIO for LED0
//!
//! @param void
//! @returns void
void gpioLed0SetOff()
{
    GPIO_PinOutClear( LED0_port, LED0_pin );
}

//! gpioLed1SetOn()
//! @brief Set GPIO for LED1
//!
//! @param void
//! @returns void
void gpioLed1SetOn()
{
    GPIO_PinOutSet( LED1_port, LED1_pin );
}

//! gpioLed1SetOff()
//! @brief Clears GPIO for LED1
//!
//! @param void
//! @returns void
void gpioLed1SetOff()
{
    GPIO_PinOutClear( LED1_port, LED1_pin );
}

//! gpioLed0Toggle()
//! @brief Toggles GPIO for LED0
//!
//! @param void
//! @returns void
void gpioLed0Toggle()
{
    GPIO_PinOutToggle( LED0_port, LED0_pin );
}

//! gpioLed1Toggle()
//! @brief Toggles GPIO for LED1
//!
//! @param void
//! @returns void
void gpioLed1Toggle()
{
    GPIO_PinOutToggle( LED1_port, LED1_pin );
}

//! gpioI2cSdaDisable()
//! @brief Clear GPIO for I2C0 SDA
//!
//! @param void
//! @returns void
void gpioI2cSdaDisable()
{
    GPIO_PinOutClear( I2C0_SDA_PORT, I2C0_SDA_PIN );
}

//! gpioI2cSclDisable()
//! @brief Clear GPIO for I2C0 SCL
//!
//! @param void
//! @returns void
void gpioI2cSclDisable()
{
    GPIO_PinOutClear( I2C0_SCL_PORT, I2C0_SCL_PIN );
}

//! gpioSi7021Enable()
//! @brief Set GPIO for Si7021 sensor
//!
//! @param void
//! @returns void
void gpioSi7021Enable()
{
    GPIO_PinOutSet( SI7021_PORT, SI7021_PIN );
}

//! gpioSi7021Disable()
//! @brief Clear GPIO for Si7021 sensor
//!
//! @param void
//! @returns void
void gpioSi7021Disable()
{
    GPIO_PinOutClear( SI7021_PORT, SI7021_PIN );
}
