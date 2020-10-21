//!
//! @file gpio.h
//! @brief Functions definitions for GPIO interface
//! @version 0.1
//! 
//! @date 2020-09-24
//! @author Roberto Baquerizo (roba8460@colorado.edu)
//! 
//! @instiution University of Colorado Boulder (UCB)
//! @course ECEN 5823-001: IoT Embedded Firmware (Fall 2020)
//! @instructor David Sluiter
//! 
//! @assignment ecen5823-assignment4-baquerrj
//! 
//! @resources Utilized Silicon Labs' EMLIB peripheral libraries to implement functionality @n
//!            em_gpio.h for GPIO macros
//! 
//! @copyright All rights reserved. Distribution allowed only for the use of assignment grading. Use of code excerpts allowed at the discretion of author. Contact for permission.
//!

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
