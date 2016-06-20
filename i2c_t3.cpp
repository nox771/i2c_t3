/*
    ------------------------------------------------------------------------------------------------------
    i2c_t3 - I2C library for Teensy 3.0/3.1/LC

    - (v8) Modified 02Apr15 by Brian (nox771 at gmail.com)
        - added support for Teensy LC:
            - fully supported (Master/Slave modes, IMM/ISR/DMA operation)
            - Wire: pins 16/17 or 18/19, rate limited to I2C_RATE_1200
            - Wire1: pins 22/23, rate limited to I2C_RATE_2400
        - added timeout on acquiring bus (prevents lockup when bus cannot be acquired)
        - added setDefaultTimeout() function for setting the default timeout to apply to all commands
        - added resetBus() function for toggling SCL to release stuck Slave devices
        - added setRate(rate) function, similar to setClock(freq), but using rate specifiers (does not
                require specifying busFreq)
        - added I2C_AUTO_RETRY user define

    - (v7) Modified 09Jan15 by Brian (nox771 at gmail.com)
        - added support for F_BUS frequencies: 60MHz, 56MHz, 48MHz, 36MHz, 24MHz, 16MHz, 8MHz, 4MHz, 2MHz
        - added new rates: I2C_RATE_1800, I2C_RATE_2800, I2C_RATE_3000
        - added new priority escalation - in cases where I2C ISR is blocked by having a lower priority than
                                          calling function, the I2C will either adjust I2C ISR to a higher priority,
                                          or switch to Immediate mode as needed.
        - added new operating mode control - I2C can be set to operate in ISR mode, DMA mode (Master only),
                                             or Immediate Mode (Master only)
        - added new begin() functions to allow setting the initial operating mode:
            - begin(i2c_mode mode, uint8_t address, i2c_pins pins, i2c_pullup pullup, i2c_rate rate, i2c_op_mode opMode)
            - begin(i2c_mode mode, uint8_t address1, uint8_t address2, i2c_pins pins, i2c_pullup pullup, i2c_rate rate, i2c_op_mode opMode)
        - added new functions:
            - uint8_t setOpMode(i2c_op_mode opMode) - used to change operating mode on the fly (only when bus is idle)
            - void sendTransmission() - non-blocking Tx with implicit I2C_STOP, added for symmetry with endTransmission()
            - uint8_t setRate(uint32_t busFreq, i2c_rate rate) - used to set I2C clock dividers to get desired rate, i2c_rate argument
            - uint8_t setRate(uint32_t busFreq, uint32_t i2cFreq) - used to set I2C clock dividers to get desired SCL freq, uint32_t argument
                                                                    (quantized to nearest i2c_rate)
        - added new Wire compatibility functions:
            - void setClock(uint32_t i2cFreq) - (note: degenerate form of setRate() with busFreq == F_BUS)
            - uint8_t endTransmission(uint8_t sendStop)
            - uint8_t requestFrom(uint8_t addr, uint8_t len)
            - uint8_t requestFrom(uint8_t addr, uint8_t len, uint8_t sendStop)
        - fixed bug in Slave Range code whereby onRequest() callback occurred prior to updating rxAddr instead of after
        - fixed bug in arbitration, was missing from Master Tx mode
        - removed I2C1 defines (now included in kinetis.h)
        - removed all debug code (eliminates rbuf dependency)

    - (v6) Modified 16Jan14 by Brian (nox771 at gmail.com)
        - all new structure using dereferenced pointers instead of hardcoding. This allows functions
          (including ISRs) to be reused across multiple I2C buses.  Most functions moved to static,
          which in turn are called by inline user functions.  Added new struct (i2cData) for holding all
          bus information.
        - added support for Teensy 3.1 and I2C1 interface on pins 29/30 and 26/31.
        - added header define (I2C_BUS_ENABLE n) to control number of enabled buses (eg. both I2C0 & I2C1
          or just I2C0).  When using only I2C0 the code and ram usage will be lower.
        - added interrupt flag (toggles pin high during ISR) with independent defines for I2C0 and
          I2C1 (refer to header file), useful for logic analyzer trigger

    - (v5) Modified 09Jun13 by Brian (nox771 at gmail.com)
        - fixed bug in ISR timeout code in which timeout condition could fail to reset in certain cases
        - fixed bug in Slave mode in sda_rising_isr attach, whereby it was not getting attached on the addr byte
        - moved debug routines so they are entirely defined internal to the library (no end user code req'd)
        - debug routines now use IntervalTimer library
        - added support for range of Slave addresses
        - added getRxAddr() for Slave using addr range to determine its called address
        - removed virtual keyword from all functions (is not a base class)

    - (v1-v4) Modified 26Feb13 by Brian (nox771 at gmail.com)
        - Reworked begin function:
            - added option for pins to use (SCL:SDA on 19:18 or 16:17 - note pin order difference)
            - added option for internal pullup - as mentioned in previous code pullup is very strong,
                                                 approx 190 ohms, but is possibly useful for high speed I2C
            - added option for rates - 100kHz, 200kHz, 300kHz, 400kHz, 600kHz, 800kHz, 1MHz, 1.2MHz, <-- 24/48MHz bus
                                       1.5MHz, 2.0MHz, 2.4MHz                                        <-- 48MHz bus only
        - Removed string.h dependency (memcpy)
        - Changed Master modes to interrupt driven
        - Added non-blocking Tx/Rx routines, and status/done/finish routines:
            - sendTransmission() - non-blocking transmit
            - sendRequest() - non-blocking receive
            - status() - reports current status
            - done() - indicates Tx/Rx complete (for main loop polling if I2C is running in background)
            - finish() - loops until Tx/Rx complete or bus error
        - Added readByte()/peekByte() for uint8_t return values (note: returns 0 instead of -1 if buf empty)
        - Added fixes for Slave Rx mode - in short Slave Rx on this part is fubar
          (as proof, notice the difference in the I2Cx_FLT register in the KL25 Sub-Family parts)
            - the SDA-rising ISR hack can work but only detects STOP conditons.
              A slave Rx followed by RepSTART won't be detected since bus remains busy.
              To fix this if IAAS occurs while already in Slave Rx mode then it will
              assume RepSTART occurred and trigger onReceive callback.
        - Separated Tx/Rx buffer sizes for asymmetric devices (adjustable in i2c_t3.h)
        - Changed Tx/Rx buffer indicies to size_t to allow for large (>256 byte) buffers
        - Left debug routines in place (controlled via header defines - default is OFF).  If debug is
            enabled, note that it can easily overrun the Debug queue on large I2C transfers, yielding
            garbage output.  Adjust ringbuf size (in rbuf.h) and possibly PIT interrupt rate to adjust
            data flow to Serial (note also the buffer in Serial can overflow if written too quickly).
        - Added getError() function to return Wire error code
        - Added pinConfigure() function for changing pins on the fly (only when bus not busy)
        - Added timeouts to endTransmission(), requestFrom(), and finish()
    ------------------------------------------------------------------------------------------------------
    Some code segments derived from:
    TwoWire.cpp - TWI/I2C library for Wiring & Arduino
    Copyright (c) 2006 Nicholas Zambetti.  All right reserved.
    Modified 2012 by Todd Krein (todd@krein.org) to implement repeated starts
    ------------------------------------------------------------------------------------------------------
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#if defined(__MK20DX128__) || defined(__MK20DX256__) || defined(__MKL26Z64__) || defined(__MK64FX512__) || defined(__MK66FX1M0__) // 3.0/3.1-3.2/LC/3.4/3.5

#include "mk20dx128.h"
#include "core_pins.h"
#include "i2c_t3.h"


// ------------------------------------------------------------------------------------------------------
// Static inits
//
struct i2cStruct i2c_t3::i2cData[] =
{
    {&I2C0_A1, &I2C0_F, &I2C0_C1, &I2C0_S, &I2C0_D, &I2C0_C2,
     &I2C0_FLT, &I2C0_RA, &I2C0_SMB, &I2C0_A2, &I2C0_SLTH, &I2C0_SLTL,
     {}, 0, 0, {}, 0, 0, I2C_OP_MODE_ISR, I2C_MASTER, I2C_PINS_18_19, I2C_PULLUP_EXT, I2C_RATE_100, I2C_STOP, I2C_WAITING,
     0, 0, 0, 0, I2C_DMA_OFF, nullptr, nullptr, nullptr, 0}
#if (I2C_BUS_NUM >= 2) && (defined(__MK20DX256__) || defined(__MK64FX512__) || defined(__MK66FX1M0__)) // 3.1-3.2/3.4/3.5
   ,{&I2C1_A1, &I2C1_F, &I2C1_C1, &I2C1_S, &I2C1_D, &I2C1_C2,
     &I2C1_FLT, &I2C1_RA, &I2C1_SMB, &I2C1_A2, &I2C1_SLTH, &I2C1_SLTL,
     {}, 0, 0, {}, 0, 0, I2C_OP_MODE_ISR, I2C_MASTER, I2C_PINS_29_30, I2C_PULLUP_EXT, I2C_RATE_100, I2C_STOP, I2C_WAITING,
     0, 0, 0, 0, I2C_DMA_OFF, nullptr, nullptr, nullptr, 0}
#elif (I2C_BUS_NUM >= 2) & defined(__MKL26Z64__) // LC
   ,{&I2C1_A1, &I2C1_F, &I2C1_C1, &I2C1_S, &I2C1_D, &I2C1_C2,
     &I2C1_FLT, &I2C1_RA, &I2C1_SMB, &I2C1_A2, &I2C1_SLTH, &I2C1_SLTL,
     {}, 0, 0, {}, 0, 0, I2C_OP_MODE_ISR, I2C_MASTER, I2C_PINS_22_23, I2C_PULLUP_EXT, I2C_RATE_100, I2C_STOP, I2C_WAITING,
     0, 0, 0, 0, I2C_DMA_OFF, nullptr, nullptr, nullptr, 0}
#endif
};


// ------------------------------------------------------------------------------------------------------
// Constructor/Destructor
//
i2c_t3::i2c_t3(uint8_t i2c_bus)
{
    bus = i2c_bus;
    i2c = &i2cData[bus];
}
i2c_t3::~i2c_t3()
{
    // if DMA active, delete DMA object
    if(i2c->opMode == I2C_OP_MODE_DMA)
        delete i2c->DMA;
}


// ------------------------------------------------------------------------------------------------------
// Initialize I2C - initializes I2C as Master or address range Slave
// return: none
// parameters:
//      mode = I2C_MASTER, I2C_SLAVE
//      address1 = 1st 7bit address for specifying Slave address range (ignored for Master mode)
//      address2 = 2nd 7bit address for specifying Slave address range (ignored for Master mode)
//      pins = Wire: I2C_PINS_18_19, I2C_PINS_16_17 | Wire1(3.1): I2C_PINS_29_30, I2C_PINS_26_31 | Wire1(LC): I2C_PINS_22_23
//      pullup = I2C_PULLUP_EXT, I2C_PULLUP_INT
//      rate = I2C_RATE_100, I2C_RATE_200, I2C_RATE_300, I2C_RATE_400, I2C_RATE_600, I2C_RATE_800, I2C_RATE_1000,
//             I2C_RATE_1200, I2C_RATE_1500, I2C_RATE_1800, I2C_RATE_2000, I2C_RATE_2400, I2C_RATE_2800, I2C_RATE_3000
//      opMode = I2C_OP_MODE_IMM, I2C_OP_MODE_ISR, I2C_OP_MODE_DMA (ignored for Slave mode, defaults to ISR)
//
void i2c_t3::begin_(struct i2cStruct* i2c, uint8_t bus, i2c_mode mode, uint8_t address1, uint8_t address2,
                    i2c_pins pins, i2c_pullup pullup, i2c_rate rate, i2c_op_mode opMode)
{
    // Enable I2C internal clock
    if(bus == 0)
        SIM_SCGC4 |= SIM_SCGC4_I2C0;
    #if I2C_BUS_NUM >= 2
        if(bus == 1)
            SIM_SCGC4 |= SIM_SCGC4_I2C1;
    #endif

    i2c->currentMode = mode; // Set mode
    i2c->currentStatus = I2C_WAITING; // reset status

    // Set Master/Slave address (zeroed in Master to prevent accidental Rx when setup is changed dynamically)
    if(i2c->currentMode == I2C_MASTER)
    {
        *(i2c->C2) = I2C_C2_HDRS; // Set high drive select
        *(i2c->A1) = 0;
        *(i2c->RA) = 0;
    }
    else
    {
        *(i2c->C2) = (address2) ? (I2C_C2_HDRS|I2C_C2_RMEN) // Set high drive select and range-match enable
                                : I2C_C2_HDRS;              // Set high drive select
        // set Slave address, if two addresses are given, setup range and put lower address in A1, higher in RA
        *(i2c->A1) = (address2) ? ((address1 < address2) ? (address1<<1) : (address2<<1))
                                : (address1<<1);
        *(i2c->RA) = (address2) ? ((address1 < address2) ? (address2<<1) : (address1<<1))
                                : 0;
    }

    // Setup pins and options (note: does not "unset" unused pins if changed).  As noted in
    // original TwoWire.cpp, internal 3.0/3.1 pullup is strong (about 190 ohms), but it can
    // work if other devices on bus have strong enough pulldown devices (usually true).
    //
    pinConfigure_(i2c, bus, pins, pullup, 0);

    // Set I2C rate
    #if !((F_BUS == 60000000) || (F_BUS == 56000000) || (F_BUS == 48000000) || \
          (F_BUS == 36000000) || (F_BUS == 24000000) || (F_BUS == 16000000) || \
          (F_BUS == 8000000) || (F_BUS == 4000000) || (F_BUS == 2000000))
        #error "I2C Error: F_BUS must be one of the following rates (in MHz): 60, 56, 48, 36, 24, 16, 8, 4, 2"
    #endif
    if(i2c->currentPins == I2C_PINS_22_23)
        setRate_(i2c, F_CPU, rate); // LC Wire1 bus uses system clock (F_CPU) instead of bus clock (F_BUS)
    else
        setRate_(i2c, F_BUS, rate);

    // Set config registers and operating mode
    setOpMode_(i2c, bus, opMode);
    if(i2c->currentMode == I2C_MASTER)
        *(i2c->C1) = I2C_C1_IICEN; // Master - enable I2C (hold in Rx mode, intr disabled)
    else
        *(i2c->C1) = I2C_C1_IICEN|I2C_C1_IICIE; // Slave - enable I2C and interrupts
}


// Set Operating Mode - this configures operating mode of the I2C as either Immediate, ISR, or DMA.
//                      By default Arduino-style begin() calls will initialize to ISR mode.  This can
//                      only be called when the bus is idle (no changing mode in the middle of Tx/Rx).
//                      Note that Slave mode can only use ISR operation.
// return: 1=success, 0=fail (bus busy)
// parameters:
//      opMode = I2C_OP_MODE_ISR, I2C_OP_MODE_DMA, I2C_OP_MODE_IMM
//
uint8_t i2c_t3::setOpMode_(struct i2cStruct* i2c, uint8_t bus, i2c_op_mode opMode)
{
    if(*(i2c->S) & I2C_S_BUSY) return 0; // return immediately if bus busy

    *(i2c->C1) = I2C_C1_IICEN; // reset I2C modes, stop intr, stop DMA
    *(i2c->S) = I2C_S_IICIF | I2C_S_ARBL; // clear status flags just in case

    // Slaves can only use ISR
    if(i2c->currentMode == I2C_SLAVE) opMode = I2C_OP_MODE_ISR;

    if(opMode == I2C_OP_MODE_IMM)
    {
        i2c->opMode = I2C_OP_MODE_IMM;
    }
    if(opMode == I2C_OP_MODE_ISR || opMode == I2C_OP_MODE_DMA)
    {
        // Nested Vec Interrupt Ctrl - enable I2C interrupt
        if(bus == 0)
        {
            NVIC_ENABLE_IRQ(IRQ_I2C0);
            I2C0_INTR_FLAG_INIT; // init I2C0 interrupt flag if used
        }
        #if I2C_BUS_NUM >= 2
            if(bus == 1)
            {
                NVIC_ENABLE_IRQ(IRQ_I2C1);
                I2C1_INTR_FLAG_INIT; // init I2C1 interrupt flag if used
            }
        #endif
        if(opMode == I2C_OP_MODE_DMA)
        {
            // attempt to get a DMA Channel (if not already allocated)
            if(i2c->DMA == nullptr)
                i2c->DMA = new DMAChannel();
            // check if object created but no available channel
            if(i2c->DMA != nullptr && i2c->DMA->channel == DMA_NUM_CHANNELS)
            {
                // revert to ISR mode if no DMA channels avail
                delete i2c->DMA;
                i2c->DMA = nullptr;
                i2c->opMode = I2C_OP_MODE_ISR;
            }
            else
            {
                // DMA object has valid channel
                if(bus == 0)
                {
                    // setup static DMA settings
                    i2c->DMA->disableOnCompletion();
                    i2c->DMA->attachInterrupt(i2c0_isr);
                    i2c->DMA->interruptAtCompletion();
                    i2c->DMA->triggerAtHardwareEvent(DMAMUX_SOURCE_I2C0);
                }
                #if I2C_BUS_NUM >= 2
                    if(bus == 1)
                    {
                        // setup static DMA settings
                        i2c->DMA->disableOnCompletion();
                        i2c->DMA->attachInterrupt(i2c1_isr);
                        i2c->DMA->interruptAtCompletion();
                        i2c->DMA->triggerAtHardwareEvent(DMAMUX_SOURCE_I2C1);
                    }
                #endif
                i2c->activeDMA = I2C_DMA_OFF;
                i2c->opMode = I2C_OP_MODE_DMA;
            }
        }
        else
            i2c->opMode = I2C_OP_MODE_ISR;
    }
    return 1;
}


// Set I2C rate - reconfigures I2C frequency divider based on supplied bus freq and desired I2C freq.
//                I2C frequency in this case is quantized to an approximate I2C_RATE based on actual
//                SCL frequency measurements using 48MHz bus as a basis.  This is done for simplicity,
//                as theoretical SCL freq do not correlate to actual freq very well.
// return: 1=success, 0=fail (incompatible bus freq & I2C rate combination)
// parameters:
//      busFreq = bus frequency, typically F_BUS unless reconfigured
//      freq = desired I2C frequency (will be quantized to nearest rate)
//
uint8_t i2c_t3::setRate_(struct i2cStruct* i2c, uint32_t busFreq, uint32_t i2cFreq)
{
    i2c_rate rate;
    if(i2cFreq >= 2050000) rate = I2C_RATE_3000;
    else if(i2cFreq >= 1950000) rate = I2C_RATE_2800;
    else if(i2cFreq >= 1800000) rate = I2C_RATE_2400;
    else if(i2cFreq >= 1520000) rate = I2C_RATE_2000;
    else if(i2cFreq >= 1330000) rate = I2C_RATE_1800;
    else if(i2cFreq >= 1100000) rate = I2C_RATE_1500;
    else if(i2cFreq >=  950000) rate = I2C_RATE_1200;
    else if(i2cFreq >=  800000) rate = I2C_RATE_1000;
    else if(i2cFreq >=  650000) rate = I2C_RATE_800;
    else if(i2cFreq >=  475000) rate = I2C_RATE_600;
    else if(i2cFreq >=  350000) rate = I2C_RATE_400;
    else if(i2cFreq >=  250000) rate = I2C_RATE_300;
    else if(i2cFreq >=  150000) rate = I2C_RATE_200;
    else                        rate = I2C_RATE_100;
    return setRate_(i2c, busFreq, rate);
}


// Set I2C rate - reconfigures I2C frequency divider based on supplied bus freq and desired rate.
// return: 1=success, 0=fail (incompatible bus freq & I2C rate combination)
// parameters:
//      busFreq = bus frequency, typically F_BUS unless reconfigured
//      rate = I2C_RATE_100, I2C_RATE_200, I2C_RATE_300, I2C_RATE_400, I2C_RATE_600, I2C_RATE_800, I2C_RATE_1000,
//             I2C_RATE_1200, I2C_RATE_1500, I2C_RATE_1800, I2C_RATE_2000, I2C_RATE_2400, I2C_RATE_2800, I2C_RATE_3000
//
// For current F_CPU -> F_BUS mapping (Teensyduino 1.21), the following are the supported I2C_RATE settings.
// For a given F_BUS, if an unsupported rate is given, then the highest freq available is used (since
// unsupported rates fall off from the high end).
//
//                                              I2C_RATE (kHz)
// F_CPU    F_BUS    3000 2800 2400 2000 1800 1500 1200 1000  800  600  400  300  200  100
// -----    -----    ---------------------------------------------------------------------
//  168M     56M            y    y    y    y    y    y    y    y    y    y    y    y    y
//  144M     48M                 y    y    y    y    y    y    y    y    y    y    y    y
//  120M     60M       y    y    y    y    y    y    y    y    y    y    y    y    y    y
//   96M     48M                 y    y    y    y    y    y    y    y    y    y    y    y
//   72M     36M                           y    y    y    y    y    y    y    y    y    y
//   48M     48M                 y    y    y    y    y    y    y    y    y    y    y    y
//   24M     24M                                     y    y    y    y    y    y    y    y
//   16M     16M                                               y    y    y    y    y    y
//    8M      8M                                                         y    y    y    y
//    4M      4M                                                                   y    y
//    2M      2M                                                                        y
//
uint8_t i2c_t3::setRate_(struct i2cStruct* i2c, uint32_t busFreq, i2c_rate rate)
{
    uint8_t ret = 1;

    // Set rate and filter
    //
    if(busFreq == 60000000)
    {
        switch(rate)                                   // Freq  SCL Div
        {                                              // ----  -------
        case I2C_RATE_100:  *(i2c->F) = 0x2C; break;   // 100k    576 (actual 104k)
        case I2C_RATE_200:  *(i2c->F) = 0x24; break;   // 200k    288 (actual 208k)
        case I2C_RATE_300:  *(i2c->F) = 0x55; break;   // 300k    176 (actual 312k)
        case I2C_RATE_400:  *(i2c->F) = 0x4F; break;   // 400k    136 (actual 417k)
        case I2C_RATE_600:  *(i2c->F) = 0x19; break;   // 600k     96 (actual 536k)
        case I2C_RATE_800:  *(i2c->F) = 0x13; break;   // 800k     72 (actual 750k)
        case I2C_RATE_1000: *(i2c->F) = 0x45; break;   // 1.0M     60 (actual 909k)
        case I2C_RATE_1200: *(i2c->F) = 0x0D; break;   // 1.2M     48 (actual 1.07M)
        case I2C_RATE_1500: *(i2c->F) = 0x0B; break;   // 1.5M     40 (actual 1.2M)
        case I2C_RATE_1800: *(i2c->F) = 0x06; break;   // 1.8M     34 (actual 1.46M)
        case I2C_RATE_2000: *(i2c->F) = 0x05; break;   // 2.0M     30 (actual 1.62M)
        case I2C_RATE_2400: *(i2c->F) = 0x02; break;   // 2.4M     24 (actual 1.9M)
        case I2C_RATE_2800: *(i2c->F) = 0x01; break;   // 2.8M     22 (actual 2.0M)
        case I2C_RATE_3000: *(i2c->F) = 0x00; break;   // 3.0M     20 (actual 2.15M)
        default:            rate=I2C_RATE_3000; ret=0;
                            *(i2c->F) = 0x00; break;   // 3.0M     20
        }
        *(i2c->FLT) = 4;
    }
    else if(busFreq == 56000000)
    {
        switch(rate)                                   // Freq  SCL Div
        {                                              // ----  -------
        case I2C_RATE_100:  *(i2c->F) = 0x2C; break;   // 100k    576 (actual 97k)
        case I2C_RATE_200:  *(i2c->F) = 0x24; break;   // 200k    288 (actual 194k)
        case I2C_RATE_300:  *(i2c->F) = 0x1E; break;   // 300k    192 (actual 290k)
        case I2C_RATE_400:  *(i2c->F) = 0x4F; break;   // 400k    136 (actual 404k)
        case I2C_RATE_600:  *(i2c->F) = 0x15; break;   // 600k     88 (actual 576k)
        case I2C_RATE_800:  *(i2c->F) = 0x0F; break;   // 800k     68 (actual 750k)
        case I2C_RATE_1000: *(i2c->F) = 0x0E; break;   // 1.0M     56 (actual 909k)
        case I2C_RATE_1200: *(i2c->F) = 0x0C; break;   // 1.2M     44 (actual 1.07M)
        case I2C_RATE_1500: *(i2c->F) = 0x0A; break;   // 1.5M     36 (actual 1.3M)
        case I2C_RATE_1800: *(i2c->F) = 0x09; break;   // 1.8M     32 (actual 1.47M)
        case I2C_RATE_2000: *(i2c->F) = 0x04; break;   // 2.0M     28 (actual 1.66M)
        case I2C_RATE_2400: *(i2c->F) = 0x02; break;   // 2.4M     24 (actual 1.94M)
        case I2C_RATE_2800: *(i2c->F) = 0x00; break;   // 2.8M     20 (actual 2.15M)
        default:            rate=I2C_RATE_2800; ret=0;
                            *(i2c->F) = 0x00; break;   // 2.8M     20
        }
        *(i2c->FLT) = 4;
    }
    else if(busFreq == 48000000)
    {
        switch(rate)                                   // Freq  SCL Div
        {                                              // ----  -------
        case I2C_RATE_100:  *(i2c->F) = 0x27; break;   // 100k    480 (actual 100k)
        case I2C_RATE_200:  *(i2c->F) = 0x5A; break;   // 200k    224 (actual 202k)
        case I2C_RATE_300:  *(i2c->F) = 0x1C; break;   // 300k    144 (actual 300k)
        case I2C_RATE_400:  *(i2c->F) = 0x85; break;   // 400k    120 (actual 400k)
        case I2C_RATE_600:  *(i2c->F) = 0x14; break;   // 600k     80 (actual 576k)
        case I2C_RATE_800:  *(i2c->F) = 0x45; break;   // 800k     60 (actual 750k)
        case I2C_RATE_1000: *(i2c->F) = 0x0D; break;   // 1.0M     48 (actual 858k)
        case I2C_RATE_1200: *(i2c->F) = 0x0B; break;   // 1.2M     40 (actual 1.0M)
        case I2C_RATE_1500: *(i2c->F) = 0x09; break;   // 1.5M     32 (actual 1.2M)
        case I2C_RATE_1800: *(i2c->F) = 0x03; break;   // 1.8M     26 (actual 1.46M)
        case I2C_RATE_2000: *(i2c->F) = 0x02; break;   // 2.0M     24 (actual 1.57M)
        case I2C_RATE_2400: *(i2c->F) = 0x00; break;   // 2.4M     20 (actual 1.85M)
        default:            rate=I2C_RATE_2400; ret=0;
                            *(i2c->F) = 0x00; break;   // 2.4M     20
        }
        *(i2c->FLT) = 4;
    }
    else if(busFreq == 36000000)
    {
        switch(rate)                                   // Freq  SCL Div
        {                                              // ----  -------
        case I2C_RATE_100:  *(i2c->F) = 0x95; break;   // 100k    352
        case I2C_RATE_200:  *(i2c->F) = 0x55; break;   // 200k    176
        case I2C_RATE_300:  *(i2c->F) = 0x85; break;   // 300k    120
        case I2C_RATE_400:  *(i2c->F) = 0x15; break;   // 400k     88
        case I2C_RATE_600:  *(i2c->F) = 0x45; break;   // 600k     60
        case I2C_RATE_800:  *(i2c->F) = 0x0C; break;   // 800k     44
        case I2C_RATE_1000: *(i2c->F) = 0x0A; break;   // 1.0M     36
        case I2C_RATE_1200: *(i2c->F) = 0x05; break;   // 1.2M     30
        case I2C_RATE_1500: *(i2c->F) = 0x02; break;   // 1.5M     24
        case I2C_RATE_1800: *(i2c->F) = 0x00; break;   // 1.8M     20
        default:            rate=I2C_RATE_1800; ret=0;
                            *(i2c->F) = 0x00; break;   // 1.8M     20
        }
        *(i2c->FLT) = 3;
    }
    else if(busFreq == 24000000)
    {
        switch(rate)                                   // Freq  SCL Div
        {                                              // ----  -------
        case I2C_RATE_100:  *(i2c->F) = 0x1F; break;   // 100k    240
        case I2C_RATE_200:  *(i2c->F) = 0x85; break;   // 200k    120
        case I2C_RATE_300:  *(i2c->F) = 0x14; break;   // 300k     80
        case I2C_RATE_400:  *(i2c->F) = 0x45; break;   // 400k     60
        case I2C_RATE_600:  *(i2c->F) = 0x0B; break;   // 600k     40
        case I2C_RATE_800:  *(i2c->F) = 0x05; break;   // 800k     30
        case I2C_RATE_1000: *(i2c->F) = 0x02; break;   // 1.0M     24
        case I2C_RATE_1200: *(i2c->F) = 0x00; break;   // 1.2M     20
        default:            rate=I2C_RATE_1200; ret=0;
                            *(i2c->F) = 0x00; break;   // 1.2M     20
        }
        *(i2c->FLT) = 2;
    }
    else if(busFreq == 16000000)
    {
        switch(rate)                                   // Freq  SCL Div
        {                                              // ----  -------
        case I2C_RATE_100:  *(i2c->F) = 0x1D; break;   // 100k    160
        case I2C_RATE_200:  *(i2c->F) = 0x14; break;   // 200k     80
        case I2C_RATE_300:  *(i2c->F) = 0x43; break;   // 300k     52
        case I2C_RATE_400:  *(i2c->F) = 0x0B; break;   // 400k     40
        case I2C_RATE_600:  *(i2c->F) = 0x03; break;   // 600k     26
        case I2C_RATE_800:  *(i2c->F) = 0x00; break;   // 800k     20
        default:            rate=I2C_RATE_800; ret=0;
                            *(i2c->F) = 0x00; break;   // 800k     20
        }
        *(i2c->FLT) = 1;
    }
    else if(busFreq == 8000000)
    {
        switch(rate)                                   // Freq  SCL Div
        {                                              // ----  -------
        case I2C_RATE_100:  *(i2c->F) = 0x14; break;   // 100k     80
        case I2C_RATE_200:  *(i2c->F) = 0x0B; break;   // 200k     40
        case I2C_RATE_300:  *(i2c->F) = 0x03; break;   // 300k     26
        case I2C_RATE_400:  *(i2c->F) = 0x00; break;   // 400k     20
        default:            rate=I2C_RATE_400; ret=0;
                            *(i2c->F) = 0x00; break;   // 400k     20
        }
        *(i2c->FLT) = 1;
    }
    else if(busFreq == 4000000)
    {
        switch(rate)                                   // Freq  SCL Div
        {                                              // ----  -------
        case I2C_RATE_100:  *(i2c->F) = 0x0B; break;   // 100k     40
        case I2C_RATE_200:  *(i2c->F) = 0x00; break;   // 200k     20
        default:            rate=I2C_RATE_200; ret=0;
                            *(i2c->F) = 0x00; break;   // 200k     20
        }
        *(i2c->FLT) = 0;
    }
    else if(busFreq == 2000000)
    {
        switch(rate)                                   // Freq  SCL Div
        {                                              // ----  -------
        case I2C_RATE_100:  *(i2c->F) = 0x00; break;   // 100k     20
        default:            rate=I2C_RATE_100; ret=0;
                            *(i2c->F) = 0x00; break;   // 100k     20
        }
        *(i2c->FLT) = 0;
    }
    else
    {
        *(i2c->F) = 0x00;                              // unknown busFreq
        *(i2c->FLT) = 0;
        ret = 0;
    }

    // for LC slave mode, clear and disable STOP interrupt (it is auto enabled in ISR)
    #if defined(__MKL26Z64__) // LC
//TODO: 3.4 & 3.5 have stop detect interrupt
        if(i2c->currentMode == I2C_SLAVE)
            *(i2c->FLT) = *(i2c->FLT) & ~I2C_FLT_STOPIE;  // clear and disable STOP intr
    #endif

    // save current rate setting
    i2c->currentRate = rate;

    return ret;
}


// ------------------------------------------------------------------------------------------------------
// Configure I2C pins - reconfigures active I2C pins on-the-fly (only works when bus is idle).  If reconfig
//                      set then inactive pins will switch to input mode using same pullup configuration.
// return: 1=success, 0=fail (bus busy)
// parameters:
//      pins = Wire: I2C_PINS_18_19, I2C_PINS_16_17 | Wire1(3.1): I2C_PINS_29_30, I2C_PINS_26_31 | Wire1(LC): I2C_PINS_22_23
//      pullup = I2C_PULLUP_EXT, I2C_PULLUP_INT
//      reconfig = 1=reconfigure alternate pins, 0=do not reconfigure alternate pins (base routine only)
//
uint8_t i2c_t3::pinConfigure_(struct i2cStruct* i2c, uint8_t bus, i2c_pins pins, i2c_pullup pullup, uint8_t reconfig)
{
    if(reconfig && (*(i2c->S) & I2C_S_BUSY)) return 0; // if reconfig return immediately if bus busy (otherwise assume initial setup)

    // The I2C interfaces are associated with the pins as follows:
    // I2C0 = Wire: I2C_PINS_18_19, I2C_PINS_16_17
    // I2C1 = Wire1(3.1): I2C_PINS_29_30, I2C_PINS_26_31 | Wire1(LC): I2C_PINS_22_23
    //
    // If pins are given an impossible value (eg. I2C0 with I2C_PINS_26_31) then the default pins will be
    // used, which for I2C0 is I2C_PINS_18_19, and for I2C1 is I2C_PINS_29_30 (3.1) or I2C_PINS_22_23 (LC)
    //
    uint32_t pinConfigAlt2 = (pullup == I2C_PULLUP_EXT) ? (PORT_PCR_MUX(2)|PORT_PCR_ODE|PORT_PCR_SRE|PORT_PCR_DSE)
                                                        : (PORT_PCR_MUX(2)|PORT_PCR_PE|PORT_PCR_PS);
    #if I2C_BUS_NUM >= 2 && (defined(__MK20DX256__) || defined(__MK64FX512__) || defined(__MK66FX1M0__))
        uint32_t pinConfigAlt6 = (pullup == I2C_PULLUP_EXT) ? (PORT_PCR_MUX(6)|PORT_PCR_ODE|PORT_PCR_SRE|PORT_PCR_DSE)
                                                            : (PORT_PCR_MUX(6)|PORT_PCR_PE|PORT_PCR_PS);
    #endif
    i2c->currentPullup = pullup;

    if(bus == 0)
    {
        if(pins == I2C_PINS_16_17)
        {
            if(reconfig)
            {
                pinMode(18,((pullup == I2C_PULLUP_EXT) ? INPUT : INPUT_PULLUP));
                pinMode(19,((pullup == I2C_PULLUP_EXT) ? INPUT : INPUT_PULLUP));
            }
            i2c->currentPins = I2C_PINS_16_17;
            CORE_PIN17_CONFIG = pinConfigAlt2;
            CORE_PIN16_CONFIG = pinConfigAlt2;
        }
        else
        {
            if(reconfig)
            {
                pinMode(17,((pullup == I2C_PULLUP_EXT) ? INPUT : INPUT_PULLUP));
                pinMode(16,((pullup == I2C_PULLUP_EXT) ? INPUT : INPUT_PULLUP));
            }
            i2c->currentPins = I2C_PINS_18_19;
            CORE_PIN18_CONFIG = pinConfigAlt2;
            CORE_PIN19_CONFIG = pinConfigAlt2;
        }
    }

    #if I2C_BUS_NUM >= 2
        if(bus == 1)
        {
            #if defined(__MK20DX256__) || defined(__MK64FX512__) || defined(__MK66FX1M0__)// 3.1
                if(pins == I2C_PINS_26_31)
                {
                    if(reconfig)
                    {
                        pinMode(30,((pullup == I2C_PULLUP_EXT) ? INPUT : INPUT_PULLUP));
                        pinMode(29,((pullup == I2C_PULLUP_EXT) ? INPUT : INPUT_PULLUP));
                    }
                    i2c->currentPins = I2C_PINS_26_31;
                    CORE_PIN31_CONFIG = pinConfigAlt6;
                    CORE_PIN26_CONFIG = pinConfigAlt6;
                }
                else
                {
                    if(reconfig)
                    {
                        pinMode(31,((pullup == I2C_PULLUP_EXT) ? INPUT : INPUT_PULLUP));
                        pinMode(26,((pullup == I2C_PULLUP_EXT) ? INPUT : INPUT_PULLUP));
                    }
                    i2c->currentPins = I2C_PINS_29_30;
                    CORE_PIN30_CONFIG = pinConfigAlt2;
                    CORE_PIN29_CONFIG = pinConfigAlt2;
                }
            #elif defined(__MKL26Z64__) // LC
                i2c->currentPins = I2C_PINS_22_23;
                CORE_PIN23_CONFIG = pinConfigAlt2;
                CORE_PIN22_CONFIG = pinConfigAlt2;
            #endif
        }
    #endif

    return 1;
}


// ------------------------------------------------------------------------------------------------------
// Acquire Bus (static) - acquires bus in Master mode and escalates priority as needed, intended
//                        for internal use only
// return: 1=success, 0=fail (cannot acquire bus)
// parameters:
//      timeout = timeout in microseconds
//      forceImm = flag to indicate if immediate mode is required
//
uint8_t i2c_t3::acquireBus_(struct i2cStruct* i2c, uint8_t bus, uint32_t timeout, uint8_t& forceImm)
{
    int irqPriority, currPriority;
    elapsedMicros deltaT;

    // start timer, then take control of the bus
    if(*(i2c->C1) & I2C_C1_MST)
    {
        // we are already the bus master, so send a repeated start
        *(i2c->C1) = I2C_C1_IICEN | I2C_C1_MST | I2C_C1_RSTA | I2C_C1_TX;
    }
    else
    {
        while(timeout == 0 || deltaT < timeout)
        {
            // we are not currently the bus master, so check if bus ready
            if(!(*(i2c->S) & I2C_S_BUSY))
            {
                // become the bus master in transmit mode (send start)
                i2c->currentMode = I2C_MASTER;
                *(i2c->C1) = I2C_C1_IICEN | I2C_C1_MST | I2C_C1_TX;
                break;
            }
        }
        #if defined(I2C_AUTO_RETRY)
            // if not master and auto-retry set, then reset bus and try one last time
            if(!(*(i2c->C1) & I2C_C1_MST))
            {
                resetBus_(i2c,bus);
                if(!(*(i2c->S) & I2C_S_BUSY))
                {
                    // become the bus master in transmit mode (send start)
                    i2c->currentMode = I2C_MASTER;
                    *(i2c->C1) = I2C_C1_IICEN | I2C_C1_MST | I2C_C1_TX;
                }
            }
        #endif
        // check if not master
        if(!(*(i2c->C1) & I2C_C1_MST))
        {
            i2c->currentStatus = I2C_TIMEOUT; // bus not acquired, mark as timeout
            return 0;
        }
    }

    // For ISR operation, check if current routine has higher priority than I2C IRQ, and if so
    // either escalate priority of I2C IRQ or send I2C using immediate mode
    if(i2c->opMode == I2C_OP_MODE_ISR || i2c->opMode == I2C_OP_MODE_DMA)
    {
        currPriority = nvic_execution_priority();
        #if defined(__MK20DX128__) // 3.0
            irqPriority = NVIC_GET_PRIORITY(IRQ_I2C0);
        #elif defined(__MK20DX256__) || defined(__MKL26Z64__) || defined(__MK64FX512__) || defined(__MK66FX1M0__) // 3.1-3.2/LC/3.4/3.5
            irqPriority = NVIC_GET_PRIORITY((bus == 0) ? IRQ_I2C0 : IRQ_I2C1);
        #endif
        if(currPriority <= irqPriority)
        {
            if(currPriority < 16)
                forceImm = 1; // current priority cannot be surpassed, force Immediate mode
            else
            {
                #if defined(__MK20DX128__) // 3.0
                    NVIC_SET_PRIORITY(IRQ_I2C0, currPriority-16);
                #elif defined(__MK20DX256__) || defined(__MKL26Z64__) || defined(__MK64FX512__) || defined(__MK66FX1M0__) // 3.1-3.2/LC/3.4/3.5
                    NVIC_SET_PRIORITY((bus == 0) ? IRQ_I2C0 : IRQ_I2C1, currPriority-16);
                #endif
            }
        }
    }
    return 1;
}


// ------------------------------------------------------------------------------------------------------
// Reset Bus - toggles SCL until SDA line is released (9 clocks max).  This is used to correct
//             a hung bus in which a Slave device missed some clocks and remains stuck outputting
//             a low signal on SDA (thereby preventing START/STOP signaling).
// return: none
//
void i2c_t3::resetBus_(struct i2cStruct* i2c, uint8_t bus)
{
    uint8_t scl=0, sda=0, count=0;

    switch(i2c->currentPins)
    {
    case I2C_PINS_18_19:
        sda = 18;
        scl = 19;
        break;
    case I2C_PINS_16_17:
        sda = 17;
        scl = 16;
        break;
    case I2C_PINS_22_23:
        sda = 23;
        scl = 22;
        break;
    case I2C_PINS_29_30:
        sda = 30;
        scl = 29;
        break;
    case I2C_PINS_26_31:
        sda = 31;
        scl = 26;
        break;
    }
    if(sda && scl)
    {
        // change pin mux to digital I/O
        pinMode(sda,((i2c->currentPullup == I2C_PULLUP_EXT) ? INPUT : INPUT_PULLUP));
        digitalWrite(scl,HIGH);
        pinMode(scl,OUTPUT);

        while(digitalRead(sda) == 0 && count++ < 10)
        {
            digitalWrite(scl,LOW);
            delayMicroseconds(5);       // 10us period == 100kHz
            digitalWrite(scl,HIGH);
            delayMicroseconds(5);
        }

        // reset status
        i2c->currentStatus = I2C_WAITING; // reset status

        // reconfigure pins for I2C
        pinConfigure_(i2c, bus, i2c->currentPins, i2c->currentPullup, 0);
    }
}


// ------------------------------------------------------------------------------------------------------
// Setup Master Transmit - initialize Tx buffer for transmit to slave at address
// return: none
// parameters:
//      address = target 7bit slave address
//
void i2c_t3::beginTransmission(uint8_t address)
{
    i2c->txBuffer[0] = (address << 1); // store target addr
    i2c->txBufferLength = 1;
    clearWriteError(); // clear any previous write error
    i2c->currentStatus = I2C_WAITING; // reset status
}


// ------------------------------------------------------------------------------------------------------
// Master Transmit - blocking routine with timeout, transmits Tx buffer to slave. i2c_stop parameter can be used
//                   to indicate if command should end with a STOP(I2C_STOP) or not (I2C_NOSTOP).
// return: 0=success, 1=data too long, 2=recv addr NACK, 3=recv data NACK, 4=other error
// parameters:
//      i2c_stop = I2C_NOSTOP, I2C_STOP
//      timeout = timeout in microseconds
//
uint8_t i2c_t3::endTransmission(struct i2cStruct* i2c, uint8_t bus, i2c_stop sendStop, uint32_t timeout)
{
    sendTransmission_(i2c, bus, sendStop, timeout);

    // wait for completion or timeout
    finish_(i2c, bus, timeout);

    return getError();
}


// ------------------------------------------------------------------------------------------------------
// Send Master Transmit - non-blocking routine, starts transmit of Tx buffer to slave. i2c_stop parameter can be
//                        used to indicate if command should end with a STOP (I2C_STOP) or not (I2C_NOSTOP). Use
//                        done() or finish() to determine completion and status() to determine success/fail.
// return: none
// parameters:
//      i2c_stop = I2C_NOSTOP, I2C_STOP
//      timeout = timeout in microseconds (only used for Immediate operation)
//
void i2c_t3::sendTransmission_(struct i2cStruct* i2c, uint8_t bus, i2c_stop sendStop, uint32_t timeout)
{
    uint8_t status, forceImm=0;
    size_t idx;

    // exit immediately if sending 0 bytes
    if(i2c->txBufferLength == 0) return;

    // update timeout
    timeout = (timeout == 0) ? i2c->defTimeout : timeout;

    // clear the status flags
    #if defined(__MKL26Z64__) // LC
//TODO: 3.4 & 3.5 have stop detect interrupt
        *(i2c->FLT) |= I2C_FLT_STOPF;     // clear STOP intr
    #endif
    *(i2c->S) = I2C_S_IICIF | I2C_S_ARBL; // clear intr, arbl

    // try to take control of the bus
    if(!acquireBus_(i2c, bus, timeout, forceImm)) return;

    //
    // Immediate mode - blocking
    //
    if(i2c->opMode == I2C_OP_MODE_IMM || forceImm)
    {
        elapsedMicros deltaT;
        i2c->currentStatus = I2C_SENDING;
        i2c->currentStop = sendStop;

        for(idx=0; idx < i2c->txBufferLength && (timeout == 0 || deltaT < timeout); idx++)
        {
            // send data, wait for done
            *(i2c->D) = i2c->txBuffer[idx];
            i2c_wait_(i2c);
            status = *(i2c->S);

            // check arbitration
            if(status & I2C_S_ARBL)
            {
                i2c->currentStatus = I2C_ARB_LOST;
                *(i2c->C1) = I2C_C1_IICEN; // change to Rx mode, intr disabled (does this send STOP if ARBL flagged?)
                *(i2c->S) = I2C_S_ARBL; // clear arbl flag
                break;
            }
            // check if slave ACK'd
            else if(status & I2C_S_RXAK)
            {
                if(idx == 0)
                    i2c->currentStatus = I2C_ADDR_NAK; // NAK on Addr
                else
                    i2c->currentStatus = I2C_DATA_NAK; // NAK on Data
                *(i2c->C1) = I2C_C1_IICEN; // send STOP, change to Rx mode, intr disabled
                break;
            }
        }

        // Set final status
        if(idx < i2c->txBufferLength)
            i2c->currentStatus = I2C_TIMEOUT; // Tx incomplete, mark as timeout
        else
            i2c->currentStatus = I2C_WAITING; // Tx complete, change to waiting state

        // send STOP if configured
        if(i2c->currentStop == I2C_STOP)
            *(i2c->C1) = I2C_C1_IICEN; // send STOP, change to Rx mode, intr disabled
        else
            *(i2c->C1) = I2C_C1_IICEN | I2C_C1_MST | I2C_C1_TX; // no STOP, stay in Tx mode, intr disabled
    }
    //
    // ISR/DMA mode - non-blocking
    //
    else if(i2c->opMode == I2C_OP_MODE_ISR || i2c->opMode == I2C_OP_MODE_DMA)
    {
        // send target addr and enable interrupts
        i2c->currentStatus = I2C_SENDING;
        i2c->currentStop = sendStop;
        i2c->txBufferIndex = 0;
        if(i2c->opMode == I2C_OP_MODE_DMA && i2c->txBufferLength >= 5) // limit transfers less than 5 bytes to ISR method
        {
            // init DMA, let the hack begin
            i2c->activeDMA = I2C_DMA_ADDR;
            i2c->DMA->sourceBuffer(&i2c->txBuffer[2],i2c->txBufferLength-3); // DMA sends all except first/second/last bytes
            i2c->DMA->destination(*(i2c->D));
        }
        // start ISR
        *(i2c->C1) = I2C_C1_IICEN | I2C_C1_IICIE | I2C_C1_MST | I2C_C1_TX; // enable intr
        *(i2c->D) = i2c->txBuffer[0];
    }
}


// ------------------------------------------------------------------------------------------------------
// Master Receive - blocking routine with timeout, requests length bytes from slave at address. Receive data will
//                  be placed in the Rx buffer. i2c_stop parameter can be used to indicate if command should end
//                  with a STOP (I2C_STOP) or not (I2C_NOSTOP).
// return: #bytes received = success, 0=fail (0 length request, NAK, timeout, or bus error)
// parameters:
//      address = target 7bit slave address
//      length = number of bytes requested
//      i2c_stop = I2C_NOSTOP, I2C_STOP
//      timeout = timeout in microseconds
//
size_t i2c_t3::requestFrom_(struct i2cStruct* i2c, uint8_t bus, uint8_t addr, size_t len, i2c_stop sendStop, uint32_t timeout)
{
    // exit immediately if request for 0 bytes
    if(len == 0) return 0;

    sendRequest_(i2c, bus, addr, len, sendStop, timeout);

    // wait for completion or timeout
    if(finish_(i2c, bus, timeout))
        return i2c->rxBufferLength;
    else
        return 0; // NAK, timeout or bus error
}


// ------------------------------------------------------------------------------------------------------
// Start Master Receive - non-blocking routine, starts request for length bytes from slave at address. Receive
//                        data will be placed in the Rx buffer. i2c_stop parameter can be used to indicate if
//                        command should end with a STOP (I2C_STOP) or not (I2C_NOSTOP). Use done() or finish()
//                        to determine completion and status() to determine success/fail.
// return: none
// parameters:
//      address = target 7bit slave address
//      length = number of bytes requested
//      i2c_stop = I2C_NOSTOP, I2C_STOP
//      timeout = timeout in microseconds (only used for Immediate operation)
//
void i2c_t3::sendRequest_(struct i2cStruct* i2c, uint8_t bus, uint8_t addr, size_t len, i2c_stop sendStop, uint32_t timeout)
{
    uint8_t status, data, chkTimeout=0, forceImm=0;

    // exit immediately if request for 0 bytes or request too large
    if(len == 0) return;
    if(len > I2C_RX_BUFFER_LENGTH) { i2c->currentStatus=I2C_BUF_OVF; return; }

    i2c->reqCount = len; // store request length
    i2c->rxBufferIndex = 0; // reset buffer
    i2c->rxBufferLength = 0;
    timeout = (timeout == 0) ? i2c->defTimeout : timeout;

    // clear the status flags
    #if defined(__MKL26Z64__) // LC
//TODO: 3.4 & 3.5 have stop detect interrupt
        *(i2c->FLT) |= I2C_FLT_STOPF;     // clear STOP intr
    #endif
    *(i2c->S) = I2C_S_IICIF | I2C_S_ARBL; // clear intr, arbl

    // try to take control of the bus
    if(!acquireBus_(i2c, bus, timeout, forceImm)) return;

    //
    // Immediate mode - blocking
    //
    if(i2c->opMode == I2C_OP_MODE_IMM || forceImm)
    {
        elapsedMicros deltaT;
        i2c->currentStatus = I2C_SEND_ADDR;
        i2c->currentStop = sendStop;

        // Send target address
        *(i2c->D) = (addr << 1) | 1; // address + READ
        i2c_wait_(i2c);
        status = *(i2c->S);

        // check arbitration
        if(status & I2C_S_ARBL)
        {
            i2c->currentStatus = I2C_ARB_LOST;
            *(i2c->C1) = I2C_C1_IICEN; // change to Rx mode, intr disabled (does this send STOP if ARBL flagged?)
            *(i2c->S) = I2C_S_ARBL; // clear arbl flag
            return;
        }
        // check if slave ACK'd
        else if(status & I2C_S_RXAK)
        {
            i2c->currentStatus = I2C_ADDR_NAK; // NAK on Addr
            *(i2c->C1) = I2C_C1_IICEN; // send STOP, change to Rx mode, intr disabled
            return;
        }
        else
        {
            // Slave addr ACK, change to Rx mode
            i2c->currentStatus = I2C_RECEIVING;
            if(i2c->reqCount == 1)
                *(i2c->C1) = I2C_C1_IICEN | I2C_C1_MST | I2C_C1_TXAK; // no STOP, Rx, NAK on recv
            else
                *(i2c->C1) = I2C_C1_IICEN | I2C_C1_MST; // no STOP, change to Rx
            data = *(i2c->D); // dummy read

            // Master receive loop
            while(i2c->rxBufferLength < i2c->reqCount && i2c->currentStatus == I2C_RECEIVING)
            {
                i2c_wait_(i2c);
                chkTimeout = (timeout != 0 && deltaT >= timeout);
                // check if 2nd to last byte or timeout
                if((i2c->rxBufferLength+2) == i2c->reqCount || (chkTimeout && !i2c->timeoutRxNAK))
                {
                    *(i2c->C1) = I2C_C1_IICEN | I2C_C1_MST | I2C_C1_TXAK; // no STOP, Rx, NAK on recv
                }
                // if last byte or timeout send STOP
                if((i2c->rxBufferLength+1) >= i2c->reqCount || (chkTimeout && i2c->timeoutRxNAK))
                {
                    i2c->timeoutRxNAK = 0; // clear flag
                    // change to Tx mode
                    *(i2c->C1) = I2C_C1_IICEN | I2C_C1_MST | I2C_C1_TX;
                    // grab last data
                    data = *(i2c->D);
                    i2c->rxBuffer[i2c->rxBufferLength++] = data;
                    if(chkTimeout)
                        i2c->currentStatus = I2C_TIMEOUT; // Rx incomplete, mark as timeout
                    else
                        i2c->currentStatus = I2C_WAITING; // Rx complete, change to waiting state
                    if(i2c->currentStop == I2C_STOP) // NAK then STOP
                    {
                        delayMicroseconds(1); // empirical patch, lets things settle before issuing STOP
                        *(i2c->C1) = I2C_C1_IICEN; // send STOP, change to Rx mode, intr disabled
                    }
                    // else NAK no STOP
                }
                else
                {
                    // grab next data, not last byte, will ACK
                    data = *(i2c->D);
                    i2c->rxBuffer[i2c->rxBufferLength++] = data;
                }
                if(chkTimeout) i2c->timeoutRxNAK = 1; // set flag to indicate NAK sent
            }
        }
    }
    //
    // ISR/DMA mode - non-blocking
    //
    else if(i2c->opMode == I2C_OP_MODE_ISR || i2c->opMode == I2C_OP_MODE_DMA)
    {
        // send 1st data and enable interrupts
        i2c->currentStatus = I2C_SEND_ADDR;
        i2c->currentStop = sendStop;
        if(i2c->opMode == I2C_OP_MODE_DMA && i2c->reqCount >= 5) // limit transfers less than 5 bytes to ISR method
        {
            // init DMA, let the hack begin
            i2c->activeDMA = I2C_DMA_ADDR;
            i2c->DMA->source(*(i2c->D));
            i2c->DMA->destinationBuffer(&i2c->rxBuffer[0],i2c->reqCount-1); // DMA gets all except last byte
        }
        // start ISR
        *(i2c->C1) = I2C_C1_IICEN | I2C_C1_IICIE | I2C_C1_MST | I2C_C1_TX; // enable intr
        *(i2c->D) = (addr << 1) | 1; // address + READ
    }
}


// ------------------------------------------------------------------------------------------------------
// Get Wire Error - returns "Wire" error code from a failed Tx/Rx command
// return: 0=success, 1=data too long, 2=recv addr NACK, 3=recv data NACK, 4=other error (timeout, arb lost)
//
uint8_t i2c_t3::getError(void)
{
    // convert status to Arduino return values (give these a higher priority than buf overflow error)
    switch(i2c->currentStatus)
    {
    case I2C_BUF_OVF:  return 1;
    case I2C_ADDR_NAK: return 2;
    case I2C_DATA_NAK: return 3;
    case I2C_ARB_LOST: return 4;
    case I2C_TIMEOUT:  return 4;
    default: break;
    }
    if(getWriteError()) return 1; // if write_error was set then flag as buffer overflow
    return 0; // no errors
}


// ------------------------------------------------------------------------------------------------------
// Done Check - returns simple complete/not-complete value to indicate I2C status
// return: 1=Tx/Rx complete (with or without errors), 0=still running
//
uint8_t i2c_t3::done_(struct i2cStruct* i2c)
{
    return (i2c->currentStatus==I2C_WAITING ||
            i2c->currentStatus==I2C_ADDR_NAK ||
            i2c->currentStatus==I2C_DATA_NAK ||
            i2c->currentStatus==I2C_ARB_LOST ||
            i2c->currentStatus==I2C_TIMEOUT ||
            i2c->currentStatus==I2C_BUF_OVF);
}


// ------------------------------------------------------------------------------------------------------
// Finish - blocking routine with timeout, loops until Tx/Rx is complete or timeout occurs
// return: 1=success (Tx or Rx completed, no error), 0=fail (NAK, Timeout, Buf Overflow, or Arb Lost)
// parameters:
//      timeout = timeout in microseconds
//
uint8_t i2c_t3::finish_(struct i2cStruct* i2c, uint8_t bus, uint32_t timeout)
{
    timeout = (timeout == 0) ? i2c->defTimeout : timeout;
    elapsedMicros deltaT;

    // wait for completion or timeout
    while(!done_(i2c) && (timeout == 0 || deltaT < timeout));

    // DMA mode and timeout
    if(timeout != 0 && deltaT >= timeout && i2c->opMode == I2C_OP_MODE_DMA && i2c->activeDMA != I2C_DMA_OFF)
    {
        // If DMA mode times out, then wait for transfer to end then mark it as timeout.
        // This is done this way because abruptly ending the DMA seems to cause
        // the I2C_S_BUSY flag to get stuck, and I cannot find a reliable way to clear it.
        while(!done_(i2c));
        i2c->currentStatus = I2C_TIMEOUT;
    }

    // check exit status, if still Tx/Rx then timeout occurred
    if(i2c->currentStatus == I2C_SENDING ||
       i2c->currentStatus == I2C_SEND_ADDR ||
       i2c->currentStatus == I2C_RECEIVING)
        i2c->currentStatus = I2C_TIMEOUT; // set to timeout state

    // delay to allow bus to settle (eg. allow STOP to complete and be recognized,
    //                               not just on our side, but on slave side also)
    delayMicroseconds(4);
    if(i2c->currentStatus == I2C_WAITING) return 1;
    return 0;
}


// ------------------------------------------------------------------------------------------------------
// Write - write data to Tx buffer
// return: #bytes written = success, 0=fail
// parameters:
//      data = data byte
//
size_t i2c_t3::write(uint8_t data)
{
    if(i2c->txBufferLength < I2C_TX_BUFFER_LENGTH)
    {
        i2c->txBuffer[i2c->txBufferLength++] = data;
        return 1;
    }
    setWriteError();
    return 0;
}


// ------------------------------------------------------------------------------------------------------
// Write Array - write length number of bytes from data array to Tx buffer
// return: #bytes written = success, 0=fail
// parameters:
//      data = pointer to uint8_t array of data
//      length = number of bytes to write
//
size_t i2c_t3::write(const uint8_t* data, size_t quantity)
{
    if(i2c->txBufferLength < I2C_TX_BUFFER_LENGTH)
    {
        size_t avail = I2C_TX_BUFFER_LENGTH - i2c->txBufferLength;
        uint8_t* dest = i2c->txBuffer + i2c->txBufferLength;

        if(quantity > avail)
        {
            quantity = avail; // truncate to space avail if needed
            setWriteError();
        }
        for(size_t count=quantity; count; count--)
            *dest++ = *data++;
        i2c->txBufferLength += quantity;
        return quantity;
    }
    setWriteError();
    return 0;
}


// ------------------------------------------------------------------------------------------------------
// Read - returns next data byte (signed int) from Rx buffer
// return: data, -1 if buffer empty
//
int i2c_t3::read_(struct i2cStruct* i2c)
{
    if(i2c->rxBufferIndex >= i2c->rxBufferLength) return -1;
    return i2c->rxBuffer[i2c->rxBufferIndex++];
}


// ------------------------------------------------------------------------------------------------------
// Peek - returns next data byte (signed int) from Rx buffer without removing it from Rx buffer
// return: data, -1 if buffer empty
//
int i2c_t3::peek_(struct i2cStruct* i2c)
{
    if(i2c->rxBufferIndex >= i2c->rxBufferLength) return -1;
    return i2c->rxBuffer[i2c->rxBufferIndex];
}


// ------------------------------------------------------------------------------------------------------
// Read Byte - returns next data byte (uint8_t) from Rx buffer
// return: data, 0 if buffer empty
//
uint8_t i2c_t3::readByte_(struct i2cStruct* i2c)
{
    if(i2c->rxBufferIndex >= i2c->rxBufferLength) return 0;
    return i2c->rxBuffer[i2c->rxBufferIndex++];
}


// ------------------------------------------------------------------------------------------------------
// Peek Byte - returns next data byte (uint8_t) from Rx buffer without removing it from Rx buffer
// return: data, 0 if buffer empty
//
uint8_t i2c_t3::peekByte_(struct i2cStruct* i2c)
{
    if(i2c->rxBufferIndex >= i2c->rxBufferLength) return 0;
    return i2c->rxBuffer[i2c->rxBufferIndex];
}


// ======================================================================================================
// ------------------------------------------------------------------------------------------------------
// I2C Interrupt Service Routine
// ------------------------------------------------------------------------------------------------------
// ======================================================================================================

// I2C0 ISR
void i2c0_isr(void)
{
    I2C0_INTR_FLAG_ON;
    i2c_isr_handler(&(i2c_t3::i2cData[0]),0);
    I2C0_INTR_FLAG_OFF;
}

#if I2C_BUS_NUM >= 2
    // I2C1 ISR
    void i2c1_isr(void)
    {
        I2C1_INTR_FLAG_ON;
        i2c_isr_handler(&(i2c_t3::i2cData[1]),1);
        I2C1_INTR_FLAG_OFF;
    }
#endif

//
// I2C ISR base handler
//
void i2c_isr_handler(struct i2cStruct* i2c, uint8_t bus)
{
    uint8_t status, c1, data;

    status = *(i2c->S);
    c1 = *(i2c->C1);
    #if defined(__MKL26Z64__)
//TODO: 3.4 & 3.5 have stop detect interrupt
        uint8_t flt = *(i2c->FLT); // LC only
    #endif

    if(c1 & I2C_C1_MST)
    {
        //
        // Master Mode
        //
        if(c1 & I2C_C1_TX)
        {
            if(i2c->activeDMA == I2C_DMA_BULK || i2c->activeDMA == I2C_DMA_LAST)
            {
                if(i2c->DMA->complete() && i2c->activeDMA == I2C_DMA_BULK)
                {
                    // clear DMA interrupt, final byte should trigger another ISR
                    i2c->DMA->clearInterrupt();
                    *(i2c->C1) = I2C_C1_IICEN | I2C_C1_IICIE | I2C_C1_MST | I2C_C1_TX; // intr en, Tx mode, DMA disabled
                    // DMA says complete at the beginning of its last byte, need to
                    // wait until end of its last byte to re-engage ISR
                    i2c->activeDMA = I2C_DMA_LAST;
                }
                else if(i2c->activeDMA == I2C_DMA_LAST)
                {
                    // wait for TCF
                    while(!(*(i2c->S) & I2C_S_TCF));
                    // clear DMA, only do this after TCF
                    i2c->DMA->clearComplete();
                    // re-engage ISR for last byte
                    i2c->activeDMA = I2C_DMA_OFF;
                    i2c->txBufferIndex = i2c->txBufferLength-1;
                    *(i2c->D) = i2c->txBuffer[i2c->txBufferIndex];
                }
                else if(i2c->DMA->error())
                {
                    i2c->DMA->clearInterrupt();
                    i2c->DMA->clearError();
                    i2c->activeDMA = I2C_DMA_OFF;
                    // check arbitration
                    if(status & I2C_S_ARBL)
                    {
                        // Arbitration Lost
                        i2c->currentStatus = I2C_ARB_LOST;
                        *(i2c->C1) = I2C_C1_IICEN; // change to Rx mode, intr disabled (does this send STOP if ARBL flagged?), DMA disabled
                        *(i2c->S) = I2C_S_ARBL | I2C_S_IICIF; // clear arbl flag and intr
                        i2c->txBufferIndex = 0; // reset Tx buffer index to prepare for resend
                        return; // does this need to check IAAS and drop to Slave Rx?
                    }
                }
                *(i2c->S) = I2C_S_IICIF; // clear intr
                return;
            } // end DMA Tx
            else
            {
                // Continue Master Transmit
                // check if Master Tx or Rx
                if(i2c->currentStatus == I2C_SENDING)
                {
                    // check arbitration
                    if(status & I2C_S_ARBL)
                    {
                        // Arbitration Lost
                        i2c->activeDMA = I2C_DMA_OFF; // clear pending DMA (if happens on address byte)
                        i2c->currentStatus = I2C_ARB_LOST;
                        *(i2c->C1) = I2C_C1_IICEN; // change to Rx mode, intr disabled (does this send STOP if ARBL flagged?)
                        *(i2c->S) = I2C_S_ARBL | I2C_S_IICIF; // clear arbl flag and intr
                        i2c->txBufferIndex = 0; // reset Tx buffer index to prepare for resend
                        return; // does this need to check IAAS and drop to Slave Rx?
                    }
                    // check if slave ACK'd
                    else if(status & I2C_S_RXAK)
                    {
                        i2c->activeDMA = I2C_DMA_OFF; // clear pending DMA (if happens on address byte)
                        if(i2c->txBufferIndex == 0)
                            i2c->currentStatus = I2C_ADDR_NAK; // NAK on Addr
                        else
                            i2c->currentStatus = I2C_DATA_NAK; // NAK on Data
                        // send STOP, change to Rx mode, intr disabled
                        // note: Slave NAK is an error, so send STOP regardless of setting
                        *(i2c->C1) = I2C_C1_IICEN;
                    }
                    else
                    {
                        // check if last byte transmitted
                        if(++i2c->txBufferIndex >= i2c->txBufferLength)
                        {
                            // Tx complete, change to waiting state
                            i2c->currentStatus = I2C_WAITING;
                            // send STOP if configured
                            if(i2c->currentStop == I2C_STOP)
                                *(i2c->C1) = I2C_C1_IICEN; // send STOP, change to Rx mode, intr disabled
                            else
                                *(i2c->C1) = I2C_C1_IICEN | I2C_C1_MST | I2C_C1_TX; // no STOP, stay in Tx mode, intr disabled
                        }
                        else if(i2c->activeDMA == I2C_DMA_ADDR)
                        {
                            // Start DMA
                            i2c->activeDMA = I2C_DMA_BULK;
                            *(i2c->C1) = I2C_C1_IICEN | I2C_C1_IICIE | I2C_C1_MST | I2C_C1_TX | I2C_C1_DMAEN; // intr en, Tx mode, DMA en
                            i2c->DMA->enable();
                            *(i2c->D) = i2c->txBuffer[1]; // DMA will start on next request
                        }
                        else
                        {
                            // ISR transmit next byte
                            *(i2c->D) = i2c->txBuffer[i2c->txBufferIndex];
                        }
                    }
                    *(i2c->S) = I2C_S_IICIF; // clear intr
                    return;
                }
                else if(i2c->currentStatus == I2C_SEND_ADDR)
                {
                    // Master Receive, addr sent
                    if(status & I2C_S_ARBL)
                    {
                        // Arbitration Lost
                        i2c->currentStatus = I2C_ARB_LOST;
                        *(i2c->C1) = I2C_C1_IICEN; // change to Rx mode, intr disabled (does this send STOP if ARBL flagged?)
                        *(i2c->S) = I2C_S_ARBL | I2C_S_IICIF; // clear arbl flag and intr
                        return;
                    }
                    else if(status & I2C_S_RXAK)
                    {
                        // Slave addr NAK
                        i2c->currentStatus = I2C_ADDR_NAK; // NAK on Addr
                        // send STOP, change to Rx mode, intr disabled
                        *(i2c->C1) = I2C_C1_IICEN;
                    }
                    else if(i2c->activeDMA == I2C_DMA_ADDR)
                    {
                        // Start DMA
                        i2c->activeDMA = I2C_DMA_BULK;
                        *(i2c->C1) = I2C_C1_IICEN | I2C_C1_IICIE | I2C_C1_MST | I2C_C1_DMAEN; // intr en, no STOP, change to Rx, DMA en
                        i2c->DMA->enable();
                        data = *(i2c->D); // dummy read
                    }
                    else
                    {
                        // Slave addr ACK, change to Rx mode
                        i2c->currentStatus = I2C_RECEIVING;
                        if(i2c->reqCount == 1)
                            *(i2c->C1) = I2C_C1_IICEN | I2C_C1_IICIE | I2C_C1_MST | I2C_C1_TXAK; // no STOP, Rx, NAK on recv
                        else
                            *(i2c->C1) = I2C_C1_IICEN | I2C_C1_IICIE | I2C_C1_MST; // no STOP, change to Rx
                        data = *(i2c->D); // dummy read
                    }
                    *(i2c->S) = I2C_S_IICIF; // clear intr
                    return;
                }
                else if(i2c->currentStatus == I2C_TIMEOUT)
                {
                    // send STOP if configured
                    if(i2c->currentStop == I2C_STOP)
                    {
                        // send STOP, change to Rx mode, intr disabled
                        *(i2c->C1) = I2C_C1_IICEN;
                    }
                    else
                    {
                        // no STOP, stay in Tx mode, intr disabled
                        *(i2c->C1) = I2C_C1_IICEN | I2C_C1_MST | I2C_C1_TX;
                    }
                    *(i2c->S) = I2C_S_IICIF; // clear intr
                    return;
                }
                else
                {
                    // Should not be in Tx mode if not sending
                    // send STOP, change to Rx mode, intr disabled
                    *(i2c->C1) = I2C_C1_IICEN;
                    *(i2c->S) = I2C_S_IICIF; // clear intr
                    return;
                }
            } // end ISR Tx
        }
        else
        {
            // Continue Master Receive
            //
            if(i2c->activeDMA == I2C_DMA_BULK || i2c->activeDMA == I2C_DMA_LAST)
            {
                if(i2c->DMA->complete() && i2c->activeDMA == I2C_DMA_BULK) // 2nd to last byte
                {
                    // clear DMA interrupt, final byte should trigger another ISR
                    i2c->DMA->clearInterrupt();
                    *(i2c->C1) = I2C_C1_IICEN | I2C_C1_IICIE | I2C_C1_MST | I2C_C1_TXAK; // intr en, Rx mode, DMA disabled, NAK on recv
                    i2c->activeDMA = I2C_DMA_LAST;
                }
                else if(i2c->activeDMA == I2C_DMA_LAST) // last byte
                {
                    // clear DMA
                    i2c->DMA->clearComplete();
                    i2c->activeDMA = I2C_DMA_OFF;
                    if(i2c->currentStatus != I2C_TIMEOUT)
                        i2c->currentStatus = I2C_WAITING; // Rx complete, change to waiting state
                    // change to Tx mode
                    *(i2c->C1) = I2C_C1_IICEN | I2C_C1_MST | I2C_C1_TX;
                    // grab last data
                    i2c->rxBufferLength = i2c->reqCount-1;
                    i2c->rxBuffer[i2c->rxBufferLength++] = *(i2c->D);
                    if(i2c->currentStop == I2C_STOP) // NAK then STOP
                    {
                        delayMicroseconds(1); // empirical patch, lets things settle before issuing STOP
                        *(i2c->C1) = I2C_C1_IICEN; // send STOP, change to Rx mode, intr disabled
                    }
                    // else NAK no STOP
                }
                else if(i2c->DMA->error()) // not sure what would cause this...
                {
                    i2c->DMA->clearError();
                    i2c->DMA->clearInterrupt();
                    i2c->activeDMA = I2C_DMA_OFF;
                    i2c->currentStatus = I2C_WAITING;
                    *(i2c->C1) = I2C_C1_IICEN; // change to Rx mode, intr disabled, DMA disabled
                }
                *(i2c->S) = I2C_S_IICIF; // clear intr
                return;
            }
            else
            {
                // check if 2nd to last byte or timeout
                if((i2c->rxBufferLength+2) == i2c->reqCount || (i2c->currentStatus == I2C_TIMEOUT && !i2c->timeoutRxNAK))
                {
                    *(i2c->C1) = I2C_C1_IICEN | I2C_C1_IICIE | I2C_C1_MST | I2C_C1_TXAK; // no STOP, Rx, NAK on recv
                }
                // if last byte or timeout send STOP
                if((i2c->rxBufferLength+1) >= i2c->reqCount || (i2c->currentStatus == I2C_TIMEOUT && i2c->timeoutRxNAK))
                {
                    i2c->timeoutRxNAK = 0; // clear flag
                    if(i2c->currentStatus != I2C_TIMEOUT)
                        i2c->currentStatus = I2C_WAITING; // Rx complete, change to waiting state
                    // change to Tx mode
                    *(i2c->C1) = I2C_C1_IICEN | I2C_C1_MST | I2C_C1_TX;
                    // grab last data
                    i2c->rxBuffer[i2c->rxBufferLength++] = *(i2c->D);
                    if(i2c->currentStop == I2C_STOP) // NAK then STOP
                    {
                        delayMicroseconds(1); // empirical patch, lets things settle before issuing STOP
                        *(i2c->C1) = I2C_C1_IICEN; // send STOP, change to Rx mode, intr disabled
                    }
                    // else NAK no STOP
                }
                else
                {
                    // grab next data, not last byte, will ACK
                    i2c->rxBuffer[i2c->rxBufferLength++] = *(i2c->D);
                }
                if(i2c->currentStatus == I2C_TIMEOUT && !i2c->timeoutRxNAK)
                    i2c->timeoutRxNAK = 1; // set flag to indicate NAK sent
                *(i2c->S) = I2C_S_IICIF; // clear intr
                return;
            }
        }
    }
    else
    {
        //
        // Slave Mode
        //
        if(status & I2C_S_ARBL)
        {
            // Arbitration Lost
            *(i2c->S) = I2C_S_ARBL; // clear arbl flag
            if(!(status & I2C_S_IAAS))
            {
                *(i2c->S) = I2C_S_IICIF; // clear intr
                return;
            }
        }
        if(status & I2C_S_IAAS)
        {
            // If in Slave Rx already, then RepSTART occured, run callback
            if(i2c->currentStatus == I2C_SLAVE_RX && i2c->user_onReceive != nullptr)
            {
                i2c->rxBufferIndex = 0;
                i2c->user_onReceive(i2c->rxBufferLength);
            }
            // Is Addressed As Slave
            if(status & I2C_S_SRW)
            {
                // Addressed Slave Transmit
                //
                i2c->currentStatus = I2C_SLAVE_TX;
                i2c->txBufferLength = 0;
                *(i2c->C1) = I2C_C1_IICEN | I2C_C1_IICIE | I2C_C1_TX;
                i2c->rxAddr = (*(i2c->D) >> 1); // read to get target addr
                if(i2c->user_onRequest != nullptr)
                    i2c->user_onRequest(); // load Tx buffer with data
                if(i2c->txBufferLength == 0)
                    i2c->txBuffer[0] = 0; // send 0's if buffer empty
                *(i2c->D) = i2c->txBuffer[0]; // send first data
                i2c->txBufferIndex = 1;
            }
            else
            {
                // Addressed Slave Receive
                //
                // setup SDA-rising ISR - required for STOP detection in Slave Rx mode for 3.0/3.1
                #if defined(__MK20DX128__) || defined(__MK20DX256__) // 3.0/3.1
                    i2c->irqCount = 0;
                    if(i2c->currentPins == I2C_PINS_18_19)
                        attachInterrupt(18, i2c_t3::sda0_rising_isr, RISING);
                    else if(i2c->currentPins == I2C_PINS_16_17)
                        attachInterrupt(17, i2c_t3::sda0_rising_isr, RISING);
                    #if I2C_BUS_NUM >= 2
                    else if(i2c->currentPins == I2C_PINS_29_30)
                        attachInterrupt(30, i2c_t3::sda1_rising_isr, RISING);
                    else if(i2c->currentPins == I2C_PINS_26_31)
                        attachInterrupt(31, i2c_t3::sda1_rising_isr, RISING);
                    #endif
                #elif defined(__MKL26Z64__)
//TODO: 3.4 & 3.5 have stop detect interrupt
                    *(i2c->FLT) |= I2C_FLT_STOPIE; // enable STOP intr for LC
                #endif
                i2c->currentStatus = I2C_SLAVE_RX;
                i2c->rxBufferLength = 0;
                *(i2c->C1) = I2C_C1_IICEN | I2C_C1_IICIE;
                i2c->rxAddr = (*(i2c->D) >> 1); // read to get target addr
            }
            *(i2c->S) = I2C_S_IICIF; // clear intr
            return;
        }
        if(c1 & I2C_C1_TX)
        {
            // Continue Slave Transmit
            if((status & I2C_S_RXAK) == 0)
            {
                // Master ACK'd previous byte
                if(i2c->txBufferIndex < i2c->txBufferLength)
                    data = i2c->txBuffer[i2c->txBufferIndex++];
                else
                    data = 0; // send 0's if buffer empty
                *(i2c->D) = data;
                *(i2c->C1) = I2C_C1_IICEN | I2C_C1_IICIE | I2C_C1_TX;
            }
            else
            {
                // Master did not ACK previous byte
                *(i2c->C1) = I2C_C1_IICEN | I2C_C1_IICIE; // switch to Rx mode
                data = *(i2c->D); // dummy read
                i2c->currentStatus = I2C_WAITING;
            }
        }
        #if defined(__MKL26Z64__) // LC
//TODO: 3.4 & 3.5 have stop detect interrupt
        else if(flt & I2C_FLT_STOPF) // STOP detected (LC only)
        {
            // There is some weird undocumented stuff going on with the I2C_FLT register on LC.
            // If you read it back, bit4 is toggling, but it is supposed to be part of FLT setting.
            // Writing just the STOPF bit causes things to get stuck, but writing back the read value
            // seems to work
            *(i2c->FLT) = flt;
            i2c->currentStatus = I2C_WAITING;
            if(i2c->user_onReceive != nullptr)
            {
                i2c->rxBufferIndex = 0;
                i2c->user_onReceive(i2c->rxBufferLength);
            }
        }
        #endif
        else
        {
            // Continue Slave Receive
            //
            // setup SDA-rising ISR - required for STOP detection in Slave Rx mode for 3.0/3.1
            #if defined(__MK20DX128__) || defined(__MK20DX256__) || defined(__MK64FX512__) || defined(__MK66FX1M0__) // 3.0/3.1
                i2c->irqCount = 0;
                if(i2c->currentPins == I2C_PINS_18_19)
                    attachInterrupt(18, i2c_t3::sda0_rising_isr, RISING);
                else if(i2c->currentPins == I2C_PINS_16_17)
                    attachInterrupt(17, i2c_t3::sda0_rising_isr, RISING);
                #if I2C_BUS_NUM >= 2
                else if(i2c->currentPins == I2C_PINS_29_30)
                    attachInterrupt(30, i2c_t3::sda1_rising_isr, RISING);
                else if(i2c->currentPins == I2C_PINS_26_31)
                    attachInterrupt(31, i2c_t3::sda1_rising_isr, RISING);
                #endif
            #elif defined(__MKL26Z64__)
//TODO: 3.4 & 3.5 have stop detect interrupt
                *(i2c->FLT) |= I2C_FLT_STOPIE; // enable STOP intr
            #endif
            data = *(i2c->D);
            if(i2c->rxBufferLength < I2C_RX_BUFFER_LENGTH)
                i2c->rxBuffer[i2c->rxBufferLength++] = data;
        }
        *(i2c->S) = I2C_S_IICIF; // clear intr
    }
}

#if defined(__MK20DX128__) || defined(__MK20DX256__) || defined(__MK64FX512__) || defined(__MK66FX1M0__) // 3.0/3.1
// ------------------------------------------------------------------------------------------------------
// SDA-Rising Interrupt Service Routine - 3.0/3.1 only
//
// Detects the stop condition that terminates a slave receive transfer.
// If anyone from Freescale ever reads this code, please email me at
// paul@pjrc.com and explain how I can respond to the I2C stop without
// inefficient polling or a horrible pin change interrupt hack?!
//

// I2C0 SDA ISR
void i2c_t3::sda0_rising_isr(void)
{
    i2c_t3::sda_rising_isr_handler(&(i2c_t3::i2cData[0]),0);
}

#if I2C_BUS_NUM >= 2
    // I2C1 SDA ISR
    void i2c_t3::sda1_rising_isr(void)
    {
        i2c_t3::sda_rising_isr_handler(&(i2c_t3::i2cData[1]),1);
    }
#endif

//
// SDA ISR base handler
//
void i2c_t3::sda_rising_isr_handler(struct i2cStruct* i2c, uint8_t bus)
{
    uint8_t status = *(i2c->S); // capture status first, can change if ISR is too slow
    if(!(status & I2C_S_BUSY))
    {
        i2c->currentStatus = I2C_WAITING;
        if(i2c->currentPins == I2C_PINS_18_19)
            detachInterrupt(18);
        else if(i2c->currentPins == I2C_PINS_16_17)
            detachInterrupt(17);
        #if I2C_BUS_NUM >= 2
        else if(i2c->currentPins == I2C_PINS_29_30)
            detachInterrupt(30);
        else if(i2c->currentPins == I2C_PINS_26_31)
            detachInterrupt(31);
        #endif
        if(i2c->user_onReceive != nullptr)
        {
            i2c->rxBufferIndex = 0;
            i2c->user_onReceive(i2c->rxBufferLength);
        }
    }
    else
    {
        if(++(i2c->irqCount) >= 2 || !(i2c->currentMode == I2C_SLAVE))
        {
            if(i2c->currentPins == I2C_PINS_18_19)
                detachInterrupt(18);
            else if(i2c->currentPins == I2C_PINS_16_17)
                detachInterrupt(17);
            #if I2C_BUS_NUM >= 2
            else if(i2c->currentPins == I2C_PINS_29_30)
                detachInterrupt(30);
            else if(i2c->currentPins == I2C_PINS_26_31)
                detachInterrupt(31);
            #endif
        }
    }
}
#endif // sda_rising_isr

// ------------------------------------------------------------------------------------------------------
// Instantiate
//
i2c_t3 Wire  = i2c_t3(0);       // I2C0
#if I2C_BUS_NUM >= 2
    i2c_t3 Wire1 = i2c_t3(1);   // I2C1
#endif

#endif // i2c_t3
