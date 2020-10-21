/*
 * @file i2c.c
 *
 *  Created on: Sep 15, 2020
 *      Author: rober
 */

#include "i2c.h"

#include "log.h"
#include "gpio.h"
#include "timers.h"

#include "i2cspm.h"
#include "em_cmu.h"

//! Data structure used to configure the I2C controller
static I2CSPM_Init_TypeDef i2cConfiguration = {
        .port      = I2C0,
        .sclPort     = gpioPortC,
        .sclPin     = 10,
        .sdaPort     = gpioPortC,
        .sdaPin     = 11,
        .portLocationScl = 14,
        .portLocationSda = 16,
        .i2cRefFreq   = 0,
        .i2cMaxFreq   = I2C_FREQ_STANDARD_MAX,
        .i2cClhr     = i2cClockHLRStandard
};

//! i2cInit()
//! @brief Initialize I2C0 device
//!
//! @param void
//! @returns void
void i2cInit()
{
  // Initialize I2C peripheral
  I2CSPM_Init( &i2cConfiguration );
  LOG_DEBUG( "exiting" );
  return;
}

//! i2cDeinit()
//! @brief Unitialize I2C0 device by disabling I2C controller,
//! disabling GPIOs for SCL/SDA, and disabling I2C clock
//! @resources this code comes from an instructor example
//!
//! @param void
//! @returns void
void i2cDeinit()
{
    // Disable the I2C controller
    I2C_Reset( I2C0 );
    I2C_Enable( I2C0, false );
    // Disable the GPIOs for SCL/SDA
    gpioI2cSdaDisable();
    gpioI2cSclDisable();
    // Turn off I2C clock in CMU, enable = false
    CMU_ClockEnable( cmuClock_I2C0, false );
    LOG_DEBUG( "exiting" );
    return;
}

//! i2cRead()
//! @brief Receive data from I2C slave
//!
//! @param[in]  *result
//! @param[in]  resultLen
//! @returns i2cTransferDone if successful, anything else is an error
I2C_TransferReturn_TypeDef i2cRead( uint8_t *result, uint16_t resultLen )
{
    I2C_TransferSeq_TypeDef seq;
    I2C_TransferReturn_TypeDef ret;

    seq.addr = SI7021_ADDR << 1;
    seq.flags = I2C_FLAG_READ;
    seq.buf[ 0 ].data = result;
    seq.buf[ 0 ].len = resultLen;


    ret = I2CSPM_Transfer( I2C0, &seq );

    LOG_DEBUG( "exiting" );
    return ret;
}

//! i2cWrite()
//! @brief Send data (a command) to I2C slave
//!
//! @param[in]  *cmd
//! @param[in]  cmdLen
//! @returns i2cTransferDone if successful, anything else is an error
I2C_TransferReturn_TypeDef i2cWrite( uint8_t *cmd, uint16_t cmdLen )
{
    I2C_TransferSeq_TypeDef seq;
    I2C_TransferReturn_TypeDef ret;

    seq.addr = SI7021_ADDR << 1;
    seq.flags = I2C_FLAG_WRITE;
    seq.buf[ 0 ].data = cmd;
    seq.buf[ 0 ].len = cmdLen;

    ret = I2CSPM_Transfer( I2C0, &seq );

    LOG_DEBUG( "exiting" );
    return ret;
}

//! i2cGetTemperature()
//! @brief Performs a temperature measurement using Si7021 sensor
//!
//! @param[in] *temperature
//! @returns true if successful, false if encountered an error
bool i2cGetTemperature( double *temperature )
{
    bool success = true;
    // initialize I2C peripheral
    i2cInit();

     // sensor enable
    gpioSi7021Enable();

    // wait 80ms for sensor power-up sequencer
    timerWaitUs( 80000 );

    I2C_TransferReturn_TypeDef status;

    buffer.data[ 0 ] = TEMP_READ_CMD;
    buffer.len = 1;
    status = i2cWrite( buffer.data, buffer.len );

    timerWaitUs( 10000 );

    uint8_t tries = 10;
    buffer.len = 2;
    while( tries-- )
    {
        status = i2cRead( buffer.data, buffer.len );

        if( i2cTransferDone == status )
        {
            *temperature = ( ( uint32_t ) buffer.data[ 0 ] << 8 ) | ( buffer.data[ 1 ] & 0xFC );
            *temperature = ( ( 175.72 * ( *temperature ) ) / 65536 ) - 46.85;
            LOG_DEBUG( "buffer.data [0x%X%X]", buffer.data[0], buffer.data[1] );
            break;
        }
        else
        {
            i2cReturnDecode( status );
            timerWaitUs( 10000 );
        }
    }
    if( tries == 0 || ( i2cTransferDone != status ) )
    {
        success = false;
    }

    // disable sensor and I2C peripheral
    gpioSi7021Disable();
    i2cDeinit();

    LOG_DEBUG( "exiting" );
    return success;
}

//! i2cReturnDecode()
//! @brief Decode an I2C_TransferReturn_Typdef return value to log the
//! the value appropriately  (LOG_ERROR, etc.)
//!
//! @param[in] _retVal
//! @returns void
void i2cReturnDecode( I2C_TransferReturn_TypeDef _retVal )
{
    switch( _retVal )
    {
        case i2cTransferDone:
        {
            LOG_DEBUG( "I2C Transfer Completed" );
            break;
        }
        case i2cTransferNack:
        {
            LOG_ERROR( "I2CSPM_Transfer() returned NACK" );
            break;
        }
        case i2cTransferBusErr:
        {
            LOG_ERROR( "I2CSPM_Transfer() returned BUSERR" );
            break;
        }
        case i2cTransferArbLost:
        {
            LOG_ERROR( "I2CSPM_Transfer() returned ARB LOST" );
            break;
        }
        case i2cTransferUsageFault:
        {
            LOG_ERROR( "I2CSPM_Transfer() returned USAGE FAULT" );
            break;
        }
        case i2cTransferSwFault:
        {
            LOG_ERROR( "I2CSPM_Transfer() returned SW FAULT" );
            break;
        }
        case i2cTransferInProgress:
        {
            LOG_WARN( "I2CSPM_Transfer() returned TRANSFER IN PROGRESS" );
            break;
        }
        default:
        {
            LOG_WARN( "Unknown return value from I2CSPM_Transfer()!" );
            break;
        }
    }
    return;
}
