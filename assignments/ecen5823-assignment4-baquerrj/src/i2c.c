//!
//! @file i2c.c
//! @brief I2C Driver functions
//!
//! @date Sep 15, 2020
//! @author Roberto Baquerizo
//!
//! @institution University of Colorado Boulder (UCB)
//! @course      ECEN 5823-001: IoT Embedded Firmware (Fall 2020)
//! @instructor  David Sluiter
//!
//! @assignment ecen5823-assignment4-baquerrj
//!
//! @resources  Utilized Silicon Labs' EMLIB peripheral libraries to implement functionality.
//!         - i2cspm.h used for i2c and Si7021 communication
//!
//! @copyright  All rights reserved. Distribution allowed only for the use of assignment grading.
//!       Use of code excerpts allowed at the discretion of author. Contact for permission.
//!

#include "i2c.h"

#include "log.h"
#include "gpio.h"
#include "timers.h"

#include "i2cspm.h"
#include "em_cmu.h"

//! Transfer Sequence structure passed to @ref I2C_TransferInit() that must 
//! exist until after I2C_Transfer() is called and the transfer is completed
static I2C_TransferSeq_TypeDef seq;

//! Local copy of data structure used for I2C Transfers. Can be accessed by
//! other modules via the i2cGetDataBuffer() routine
static i2cData_s buffer;

//! i2cGetDataBuffer()
//! @brief Performs temperature conversion on raw data by calling
//! the i2cGetTemperature() routine and returns pointer to our buffer
//!
//! @param void
//! @returns pointer to i2cData_s buffer
i2cData_s* i2cGetDataBuffer()
{
    i2cGetTemperature();
    return &buffer;
}

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


static void i2cSleepBlockStart( SLEEP_EnergyMode_t sleepMode );
static void i2cSleepBlockEnd( SLEEP_EnergyMode_t sleepMode );


void i2cEM2BlockStart()
{
    i2cSleepBlockStart( sleepEM2 );
}

//! i2cEM2BlockEnd()
//! @brief Stop blocking EM2 mode
//!
//! @param void
//! @returns void
void i2cEM2BlockEnd()
{
    i2cSleepBlockEnd( sleepEM2 );
}

//! i2cInit()
//! @brief Initialize I2C0 device and also enable I2C IRQs
//!
//! @param void
//! @returns void
void i2cInit()
{
    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_CRITICAL();
    // Initialize I2C peripheral
    I2CSPM_Init( &i2cConfiguration );
    NVIC_EnableIRQ( I2C0_IRQn );
    CORE_EXIT_CRITICAL();
    
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
    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_CRITICAL();
    // Disable the I2C controller
    I2C_Reset( I2C0 );
    I2C_Enable( I2C0, false );
    // Disable the GPIOs for SCL/SDA
    gpioI2cSdaDisable();
    gpioI2cSclDisable();
    // Turn off I2C clock in CMU, enable = false
    CMU_ClockEnable( cmuClock_I2C0, false );
    NVIC_DisableIRQ( I2C0_IRQn );
    CORE_EXIT_CRITICAL();

    return;
}

//! i2cSendCommand()
//! @brief Start transfer to command sensor to take a
//! temperature measurement
//!
//! @param void
//! @returns i2cTransferInProgress when transfer is successfully initiated
I2C_TransferReturn_TypeDef i2cSendCommand()
{
    I2C_TransferReturn_TypeDef status;
    buffer.data[ 0 ] = TEMP_READ_CMD;
    buffer.len = 1;
    status = i2cWrite( buffer.data, buffer.len );
    if( i2cTransferInProgress != status && i2cTransferDone != status )
    {
        i2cReturnDecode( status );
    }
    return status;
}

//! i2cReceiveData()
//! @brief Start transfer to receive temperature reading
//! from sensor
//!
//! @param void
//! @returns i2cTransferInProgress when transfer is successfully initiated
I2C_TransferReturn_TypeDef i2cReceiveData()
{
    I2C_TransferReturn_TypeDef status;
    buffer.len = 2;
    status = i2cRead( buffer.data, buffer.len );
    if( i2cTransferInProgress != status && i2cTransferDone != status )
    {
        i2cReturnDecode( status );
    }
    return status;
}

//! i2cRead()
//! @brief Receive data from I2C slave
//!
//! @param  *result
//! @param  resultLen
//! @returns i2cTransferInProgress if transfer successfully started.
//! Transfer is completed by calling I2C_Transfer() in interrupt context
I2C_TransferReturn_TypeDef i2cRead( uint8_t *result, uint16_t resultLen )
{
    seq.addr = SI7021_ADDR << 1;
    seq.flags = I2C_FLAG_READ;
    seq.buf[ 0 ].data = result;
    seq.buf[ 0 ].len = resultLen;

    return I2C_TransferInit( I2C0, &seq );
}

//! i2cWrite()
//! @brief Send data (a command) to I2C slave
//!
//! @param  *cmd
//! @param  cmdLen
//! @returns i2cTransferInProgress if transfer successfully started.
//! Transfer is completed by calling I2C_Transfer() in interrupt context
I2C_TransferReturn_TypeDef i2cWrite( uint8_t *cmd, uint16_t cmdLen )
{
    seq.addr = SI7021_ADDR << 1;
    seq.flags = I2C_FLAG_WRITE;
    seq.buf[ 0 ].data = cmd;
    seq.buf[ 0 ].len = cmdLen;

    return I2C_TransferInit( I2C0, &seq );
}

//! i2cGetTemperature()
//! @brief Performs a temperature measurement using Si7021 sensor
//!
//! @param void
//! @returns temperature in degrees Celsius
double i2cGetTemperature()
{
    buffer.temperature = 0;
    buffer.temperature = ((uint32_t)buffer.data[0] << 8) | (buffer.data[1] & 0xFC);
    buffer.temperature = ((175.72 * (buffer.temperature)) / 65536) - 46.85;
    LOG_DEBUG("buffer.data [0x%X%X]", buffer.data[0], buffer.data[1]);

    return buffer.temperature;
}

//! i2cReturnDecode()
//! @brief Decode an I2C_TransferReturn_Typdef return value to log the
//! the value appropriately  (LOG_ERROR, etc.)
//!
//! @param _retVal
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

//! i2cSleepBlockStart()
//! @brief Private wrapper for I2C module to @ref SLEEP_SleepBlockBegin()
//! Makes CRITICAL section macro API from @ref em_core.h
//!
//! @param sleepMode energy mode to block
//! @returns void
static void i2cSleepBlockStart( SLEEP_EnergyMode_t sleepMode )
{
    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_CRITICAL();
    SLEEP_SleepBlockBegin( sleepMode );
    CORE_EXIT_CRITICAL();
}

//! i2cSleepBlockEnd()
//! @brief Private wrapper for I2C module to @ref SLEEP_SleepBlockEnd()
//! Makes CRITICAL section macro API from @ref em_core.h
//!
//! @param sleepMode energy mode to block
//! @returns void
static void i2cSleepBlockEnd( SLEEP_EnergyMode_t sleepMode )
{
    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_CRITICAL();
    SLEEP_SleepBlockEnd( sleepMode );
    CORE_EXIT_CRITICAL();
}
