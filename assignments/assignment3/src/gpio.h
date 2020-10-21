/*
 * gpio.h
 *
 *  Created on: Dec 12, 2018
 *      Author: Dan Walkes
 */

#ifndef SRC_GPIO_H_
#define SRC_GPIO_H_

#include <stdbool.h>
#include "em_gpio.h"

//! Macros defining port and pin assignments for various peripherals
#define SI7021_PORT     (gpioPortD)
#define SI7021_PIN      (15)
#define LED0_port       (gpioPortF)
#define LED0_pin        (4)
#define LED1_port       (gpioPortF)
#define LED1_pin        (5)
#define I2C0_SCL_PORT   (gpioPortC)
#define I2C0_SCL_PIN    (10)
#define I2C0_SDA_PORT   (gpioPortC)
#define I2C0_SDA_PIN    (11)

void gpioInit();
void gpioLed0SetOn();
void gpioLed0SetOff();
void gpioLed1SetOn();
void gpioLed1SetOff();
void gpioLed0Toggle();
void gpioLed1Toggle();

void gpioI2cSdaDisable();
void gpioI2cSclDisable();
void gpioSi7021Enable();
void gpioSi7021Disable();

#endif /* SRC_GPIO_H_ */
