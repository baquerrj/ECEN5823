/*
 * @file i2c.h
 *
 *  Created on: Sep 15, 2020
 *      Author: rober
 */

#ifndef __I2C_H___
#define __I2C_H___

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "em_gpio.h"
#include "em_i2c.h"

//! @resources Si7021-A20
//! I2C slave address for Si7021 sensor
static const uint8_t SI7021_ADDR = 0x40;

//! Measure Temperature, No Hold Master Mode Command
static const uint8_t TEMP_READ_CMD = 0xF3;

//! Data structure to contain I2C commands or data from slave
typedef struct
{
        uint8_t  data[ 2 ];
        uint16_t len;
} i2cData_s;

i2cData_s buffer;

void i2cInit();

void i2cDeinit();

I2C_TransferReturn_TypeDef i2cRead( uint8_t *result, uint16_t resultLen );

I2C_TransferReturn_TypeDef i2cWrite( uint8_t *cmd, uint16_t cmdLen );

bool i2cGetTemperature( double *temperature );

void i2cReturnDecode( I2C_TransferReturn_TypeDef _retVal );

#endif // __I2C_H___
