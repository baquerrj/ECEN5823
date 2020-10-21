//!
//! @file i2c.h
//! @brief I2C Driver functions and types definitions
//! @version 0.1
//! 
//! @date 2020-09-24
//! @author Roberto Baquerizo (roba8460@colorado.edu)
//! 
//! @institution University of Colorado Boulder (UCB)
//! @course ECEN 5823-001: IoT Embedded Firmware (Fall 2020)
//! @instructor David Sluiter
//! 
//! @assignment ecen5823-assignment6-baquerrj
//! 
//! @resources Utilized Silicon Labs' EMLIB peripheral libraries to implement functionality
//! 
//! @copyright All rights reserved. Distribution allowed only for the use of assignment grading. Use of code excerpts allowed at the discretion of author. Contact for permission.
//!

#ifndef __I2C_H___
#define __I2C_H___

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "em_core.h"
#include "em_i2c.h"
#include "sleep.h"

//! @resources Si7021-A20
//! I2C slave address for Si7021 sensor
static const uint8_t SI7021_ADDR = 0x40;

//! Measure Temperature, Hold Master Mode Command
static const uint8_t TEMP_READ_CMD = 0xE3;

//! Measure Temperature, No Hold Master Mode Command
//static const uint8_t TEMP_READ_CMD = 0xF3;

//! Data structure to contain I2C commands or data from slave
typedef struct
{
        uint8_t  data[ 2 ];
        uint16_t len;
        union
        {
            double temperature;
            double humidity;
        };
        
} i2cData_s;

void i2cInit();

void i2cDeinit();

I2C_TransferReturn_TypeDef i2cSendCommand();

I2C_TransferReturn_TypeDef i2cReceiveData();

I2C_TransferReturn_TypeDef i2cRead( uint8_t *result, uint16_t resultLen );

I2C_TransferReturn_TypeDef i2cWrite( uint8_t *cmd, uint16_t cmdLen );

double i2cGetTemperature();

void i2cReturnDecode( I2C_TransferReturn_TypeDef _retVal );

void i2cEM2BlockStart();

void i2cEM2BlockEnd();

i2cData_s* i2cGetDataBuffer();

#endif // __I2C_H___
