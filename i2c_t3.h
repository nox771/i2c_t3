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

#if !defined(I2C_T3_H) && (defined(__MK20DX128__) || defined(__MK20DX256__) || defined(__MKL26Z64__)) // 3.0/3.1/LC
#define I2C_T3_H

#include <inttypes.h>
#include <stdio.h> // for size_t
#include "Arduino.h"
#include <DMAChannel.h>

// ======================================================================================================
// == Start User Define Section =========================================================================
// ======================================================================================================

// ------------------------------------------------------------------------------------------------------
// I2C Bus Enable control - change to enable buses as follows.  This affects Teensy3.1 and LC devices.
//                          Teensy3.0 always uses I2C0 only regardless of this setting.
//
// I2C_BUS_ENABLE 1   (enable I2C0 only)
// I2C_BUS_ENABLE 2   (enable I2C0 & I2C1)
//
#define I2C_BUS_ENABLE 2

// ------------------------------------------------------------------------------------------------------
// Tx/Rx buffer sizes - modify these as needed.  Buffers should be large enough to hold:
//                      Target Addr + Target Command (varies with protocol) + Data payload
//                      Default is: 1byte Addr + 2byte Command + 256byte Data
//
#define I2C_TX_BUFFER_LENGTH 259
#define I2C_RX_BUFFER_LENGTH 259

// ------------------------------------------------------------------------------------------------------
// Interrupt flag - uncomment and set below to make the specified pin high whenever the
//                  I2C interrupt occurs.  This is useful as a trigger signal when using a logic analyzer.
//
//#define I2C0_INTR_FLAG_PIN 6
//#define I2C1_INTR_FLAG_PIN 7

// ------------------------------------------------------------------------------------------------------
// Auto retry - uncomment to make the library automatically call resetBus() if it has a timeout while
//              trying to send a START (occurs at the beginning of any endTransmission() or requestFrom()
//              call).  This will toggle SCL to try and get a hung Slave device to release the SDA line.
//              If successful then it will try again to send a START, if not then it will return a timeout
//              error (same as if auto retry was not defined).
//
// Note: this is incompatible with multi-master buses, only use in single-master configurations
//
#define I2C_AUTO_RETRY

// ======================================================================================================
// == End User Define Section ===========================================================================
// ======================================================================================================


// ------------------------------------------------------------------------------------------------------
// Set number of enabled buses
//
#if (defined(__MK20DX256__) || defined(__MKL26Z64__)) && (I2C_BUS_ENABLE >= 2) // 3.1/LC
    #define I2C_BUS_NUM 2
#else
    #define I2C_BUS_NUM 1
#endif


// ------------------------------------------------------------------------------------------------------
// Interrupt flag setup
//
#if defined(I2C0_INTR_FLAG_PIN)
    #define I2C0_INTR_FLAG_INIT do             \
    {                                          \
        pinMode(I2C0_INTR_FLAG_PIN, OUTPUT);   \
        digitalWrite(I2C0_INTR_FLAG_PIN, LOW); \
    } while(0)

    #define I2C0_INTR_FLAG_ON   do {digitalWrite(I2C0_INTR_FLAG_PIN, HIGH);} while(0)
    #define I2C0_INTR_FLAG_OFF  do {digitalWrite(I2C0_INTR_FLAG_PIN, LOW);} while(0)
#else
    #define I2C0_INTR_FLAG_INIT do{}while(0)
    #define I2C0_INTR_FLAG_ON   do{}while(0)
    #define I2C0_INTR_FLAG_OFF  do{}while(0)
#endif

#if defined(I2C1_INTR_FLAG_PIN)
    #define I2C1_INTR_FLAG_INIT do             \
    {                                          \
        pinMode(I2C1_INTR_FLAG_PIN, OUTPUT);   \
        digitalWrite(I2C1_INTR_FLAG_PIN, LOW); \
    } while(0)

    #define I2C1_INTR_FLAG_ON   do {digitalWrite(I2C1_INTR_FLAG_PIN, HIGH);} while(0)
    #define I2C1_INTR_FLAG_OFF  do {digitalWrite(I2C1_INTR_FLAG_PIN, LOW);} while(0)
#else
    #define I2C1_INTR_FLAG_INIT do{}while(0)
    #define I2C1_INTR_FLAG_ON   do{}while(0)
    #define I2C1_INTR_FLAG_OFF  do{}while(0)
#endif


// ------------------------------------------------------------------------------------------------------
// Function argument enums
//
enum i2c_op_mode  {I2C_OP_MODE_IMM, I2C_OP_MODE_ISR, I2C_OP_MODE_DMA};
enum i2c_mode     {I2C_MASTER, I2C_SLAVE};
enum i2c_pins     {I2C_PINS_18_19,          // 19 SCL  18 SDA
                   I2C_PINS_16_17,          // 16 SCL  17 SDA
                   I2C_PINS_22_23,          // 22 SCL  23 SDA  (LC only)
                   I2C_PINS_29_30,          // 29 SCL  30 SDA  (3.1 only)
                   I2C_PINS_26_31};         // 26 SCL  31 SDA  (3.1 only)
enum i2c_pullup   {I2C_PULLUP_EXT, I2C_PULLUP_INT};
enum i2c_rate     {I2C_RATE_100,
                   I2C_RATE_200,
                   I2C_RATE_300,
                   I2C_RATE_400,
                   I2C_RATE_600,
                   I2C_RATE_800,
                   I2C_RATE_1000,
                   I2C_RATE_1200,
                   I2C_RATE_1500,
                   I2C_RATE_1800,
                   I2C_RATE_2000,
                   I2C_RATE_2400,
                   I2C_RATE_2800,
                   I2C_RATE_3000};
enum i2c_stop     {I2C_NOSTOP, I2C_STOP};
enum i2c_status   {I2C_WAITING,
                   I2C_SENDING,
                   I2C_SEND_ADDR,
                   I2C_RECEIVING,
                   I2C_TIMEOUT,
                   I2C_ADDR_NAK,
                   I2C_DATA_NAK,
                   I2C_ARB_LOST,
                   I2C_BUF_OVF,
                   I2C_SLAVE_TX,
                   I2C_SLAVE_RX};
enum i2c_dma_state {I2C_DMA_OFF,
                    I2C_DMA_ADDR,
                    I2C_DMA_BULK,
                    I2C_DMA_LAST};


// ------------------------------------------------------------------------------------------------------
// Main I2C data structure
//
struct i2cStruct
{
    volatile uint8_t* A1;                    // Address Register 1                (User&ISR)
    volatile uint8_t* F;                     // Frequency Divider Register        (User&ISR)
    volatile uint8_t* C1;                    // Control Register 1                (User&ISR)
    volatile uint8_t* S;                     // Status Register                   (User&ISR)
    volatile uint8_t* D;                     // Data I/O Register                 (User&ISR)
    volatile uint8_t* C2;                    // Control Register 2                (User&ISR)
    volatile uint8_t* FLT;                   // Programmable Input Glitch Filter  (User&ISR)
    volatile uint8_t* RA;                    // Range Address Register            (User&ISR)
    volatile uint8_t* SMB;                   // SMBus Control and Status Register (User&ISR)
    volatile uint8_t* A2;                    // Address Register 2                (User&ISR)
    volatile uint8_t* SLTH;                  // SCL Low Timeout Register High     (User&ISR)
    volatile uint8_t* SLTL;                  // SCL Low Timeout Register Low      (User&ISR)
    uint8_t  rxBuffer[I2C_RX_BUFFER_LENGTH]; // Rx Buffer                         (ISR)
    volatile size_t   rxBufferIndex;         // Rx Index                          (User&ISR)
    volatile size_t   rxBufferLength;        // Rx Length                         (ISR)
    uint8_t  txBuffer[I2C_TX_BUFFER_LENGTH]; // Tx Buffer                         (User)
    volatile size_t   txBufferIndex;         // Tx Index                          (User&ISR)
    volatile size_t   txBufferLength;        // Tx Length                         (User&ISR)
    i2c_op_mode opMode;                      // Operating Mode                    (User)
    i2c_mode currentMode;                    // Current Mode                      (User)
    i2c_pins currentPins;                    // Current Pins                      (User)
    i2c_pullup currentPullup;                // Current Pullup                    (User)
    i2c_rate currentRate;                    // Current Rate                      (User)
    i2c_stop currentStop;                    // Current Stop                      (User)
    volatile i2c_status currentStatus;       // Current Status                    (User&ISR)
    uint8_t  rxAddr;                         // Rx Address                        (ISR)
    size_t   reqCount;                       // Byte Request Count                (User)
    uint8_t  irqCount;                       // IRQ Count, used by SDA-rising ISR (ISR)
    uint8_t  timeoutRxNAK;                   // Rx Timeout NAK flag               (ISR)
    volatile i2c_dma_state activeDMA;        // Active DMA flag                   (User&ISR)
    void (*user_onReceive)(size_t len);      // Slave Rx Callback Function        (User)
    void (*user_onRequest)(void);            // Slave Tx Callback Function        (User)
    DMAChannel* DMA;                         // DMA Channel object                (User&ISR)
    uint32_t defTimeout;                     // Default Timeout                   (User)
};


// ------------------------------------------------------------------------------------------------------
// I2C Class
//
extern "C" void i2c0_isr(void);
extern "C" void i2c1_isr(void);
extern "C" void i2c_isr_handler(struct i2cStruct* i2c, uint8_t bus);

class i2c_t3 : public Stream
{
private:
    //
    // I2C data structures - these need to be static so "C" ISRs can use them
    //
    static struct i2cStruct i2cData[I2C_BUS_NUM];
    //
    // Primary I2C ISR (I2C0)
    //
    friend void i2c0_isr(void);
    friend void i2c_isr_handler(struct i2cStruct* i2c, uint8_t bus);
    //
    // Slave STOP detection (I2C0) - 3.0/3.1 only
    //
    #if (defined(__MK20DX128__) || defined(__MK20DX256__))
        static void sda0_rising_isr(void);
        static void sda_rising_isr_handler(struct i2cStruct* i2c, uint8_t bus);
    #endif

    #if I2C_BUS_NUM >= 2
        //
        // Secondary I2C ISR (I2C1)
        //
        friend void i2c1_isr(void);
        //
        // Slave STOP detection (I2C1) - 3.1 only
        //
        #if defined(__MK20DX256__)
            static void sda1_rising_isr(void);
        #endif
    #endif

public:
    //
    // I2C bus number - this is a local, passed as an argument to base functions
    //                  since static functions cannot see it.
    uint8_t bus;
    //
    // I2C structure pointer - this is a local, passed as an argument to base functions
    //                         since static functions cannot see it.
    struct i2cStruct* i2c;

    // ------------------------------------------------------------------------------------------------------
    // Constructor
    //
    i2c_t3(uint8_t i2c_bus);
    ~i2c_t3();

    // ------------------------------------------------------------------------------------------------------
    // Initialize I2C (base routine)
    //
    static void begin_(struct i2cStruct* i2c, uint8_t bus, i2c_mode mode, uint8_t address1, uint8_t address2,
                       i2c_pins pins, i2c_pullup pullup, i2c_rate rate, i2c_op_mode opMode);
    //
    // Initialize I2C (Master) - initializes I2C as Master mode, external pullups, 100kHz rate
    //                         - pins 18/19 (Wire), pins 29/30 (Wire1 on 3.1), pins 22/23 (Wire1 on LC)
    // return: none
    //
    inline void begin()
    {
        #if defined(__MKL26Z64__)
            begin_(i2c, bus, I2C_MASTER, 0, 0, ((bus == 0) ? I2C_PINS_18_19 : I2C_PINS_22_23), I2C_PULLUP_EXT, I2C_RATE_100, I2C_OP_MODE_ISR); // LC
        #else
            begin_(i2c, bus, I2C_MASTER, 0, 0, ((bus == 0) ? I2C_PINS_18_19 : I2C_PINS_29_30), I2C_PULLUP_EXT, I2C_RATE_100, I2C_OP_MODE_ISR); // 3.0/3.1
        #endif
    }
    //
    // Initialize I2C (Slave) - initializes I2C as Slave mode using address, external pullups, 100kHz rate
    //                        - pins 18/19 (Wire), pins 29/30 (Wire1 on 3.1), pins 22/23 (Wire1 on LC)
    // return: none
    // parameters:
    //      address = 7bit slave address of device
    //
    inline void begin(int address)
    {
        #if defined(__MKL26Z64__)
            begin_(i2c, bus, I2C_SLAVE, (uint8_t)address, 0, ((bus == 0) ? I2C_PINS_18_19 : I2C_PINS_22_23), I2C_PULLUP_EXT, I2C_RATE_100, I2C_OP_MODE_ISR); // LC
        #else
            begin_(i2c, bus, I2C_SLAVE, (uint8_t)address, 0, ((bus == 0) ? I2C_PINS_18_19 : I2C_PINS_29_30), I2C_PULLUP_EXT, I2C_RATE_100, I2C_OP_MODE_ISR); // 3.0/3.1
        #endif
    }
    inline void begin(uint8_t address)
    {
        #if defined(__MKL26Z64__)
            begin_(i2c, bus, I2C_SLAVE, address, 0, ((bus == 0) ? I2C_PINS_18_19 : I2C_PINS_22_23), I2C_PULLUP_EXT, I2C_RATE_100, I2C_OP_MODE_ISR); // LC
        #else
            begin_(i2c, bus, I2C_SLAVE, address, 0, ((bus == 0) ? I2C_PINS_18_19 : I2C_PINS_29_30), I2C_PULLUP_EXT, I2C_RATE_100, I2C_OP_MODE_ISR); // 3.0/3.1
        #endif
    }
    //
    // Initialize I2C - initializes I2C as Master or single address Slave
    // return: none
    // parameters:
    //      mode = I2C_MASTER, I2C_SLAVE
    //      address = 7bit slave address when configured as Slave (ignored for Master mode)
    //      pins = Wire: I2C_PINS_18_19, I2C_PINS_16_17 | Wire1(3.1): I2C_PINS_29_30, I2C_PINS_26_31 | Wire1(LC): I2C_PINS_22_23
    //      pullup = I2C_PULLUP_EXT, I2C_PULLUP_INT
    //      rate = I2C_RATE_100, I2C_RATE_200, I2C_RATE_300, I2C_RATE_400, I2C_RATE_600, I2C_RATE_800, I2C_RATE_1000,
    //             I2C_RATE_1200, I2C_RATE_1500, I2C_RATE_1800, I2C_RATE_2000, I2C_RATE_2400, I2C_RATE_2800, I2C_RATE_3000
    //      opMode = I2C_OP_MODE_IMM, I2C_OP_MODE_ISR, I2C_OP_MODE_DMA (ignored for Slave mode, defaults to ISR)
    //
    inline void begin(i2c_mode mode, uint8_t address, i2c_pins pins, i2c_pullup pullup, i2c_rate rate, i2c_op_mode opMode=I2C_OP_MODE_ISR)
        { begin_(i2c, bus, mode, address, 0, pins, pullup, rate, opMode); }
    //
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
    inline void begin(i2c_mode mode, uint8_t address1, uint8_t address2, i2c_pins pins, i2c_pullup pullup, i2c_rate rate, i2c_op_mode opMode=I2C_OP_MODE_ISR)
        { begin_(i2c, bus, mode, address1, address2, pins, pullup, rate, opMode); }

    // ------------------------------------------------------------------------------------------------------
    // Set Operating Mode (base routine)
    //
    static uint8_t setOpMode_(struct i2cStruct* i2c, uint8_t bus, i2c_op_mode opMode);
    //
    // Set Operating Mode - this configures operating mode of the I2C as either Immediate, ISR, or DMA.
    //                      By default Arduino-style begin() calls will initialize to ISR mode.  This can
    //                      only be called when the bus is idle (no changing mode in the middle of Tx/Rx).
    //                      Note that Slave mode can only use ISR operation.
    // return: 1=success, 0=fail (bus busy)
    // parameters:
    //      opMode = I2C_OP_MODE_ISR, I2C_OP_MODE_DMA, I2C_OP_MODE_IMM
    //
    inline uint8_t setOpMode(i2c_op_mode opMode) { return setOpMode_(i2c, bus, opMode); }

    // ------------------------------------------------------------------------------------------------------
    // Set I2C rate (base routines)
    //
    static uint8_t setRate_(struct i2cStruct* i2c, uint32_t busFreq, i2c_rate rate);
    static uint8_t setRate_(struct i2cStruct* i2c, uint32_t busFreq, uint32_t i2cFreq);
    //
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
    inline uint8_t setRate(uint32_t busFreq, i2c_rate rate) { return setRate_(i2c, busFreq, rate); }
    inline uint8_t setRate(i2c_rate rate) // i2c_t3 version of setClock
    {
        if(i2c->currentPins == I2C_PINS_22_23)
            return setRate_(i2c, (uint32_t)F_CPU, rate); // LC Wire1 bus uses system clock (F_CPU) instead of bus clock (F_BUS)
        else
            return setRate_(i2c, (uint32_t)F_BUS, rate);
    }
    //
    // Set I2C rate - reconfigures I2C frequency divider based on supplied bus freq and desired I2C freq.
    //                I2C frequency in this case is quantized to an approximate I2C_RATE based on actual
    //                SCL frequency measurements using 48MHz bus as a basis.  This is done for simplicity,
    //                as theoretical SCL freq do not correlate to actual freq very well.
    // return: 1=success, 0=fail (incompatible bus freq & I2C rate combination)
    // parameters:
    //      busFreq = bus frequency, typically F_BUS unless reconfigured
    //      freq = desired I2C frequency (will be quantized to nearest rate)
    //
    inline uint8_t setRate(uint32_t busFreq, uint32_t i2cFreq) { return setRate_(i2c, busFreq, i2cFreq); }
    inline void setClock(uint32_t i2cFreq) // Wire compatibility
    {
        if(i2c->currentPins == I2C_PINS_22_23)
            setRate_(i2c, (uint32_t)F_CPU, i2cFreq); // LC Wire1 bus uses system clock (F_CPU) instead of bus clock (F_BUS)
        else
            setRate_(i2c, (uint32_t)F_BUS, i2cFreq);
    }

    // ------------------------------------------------------------------------------------------------------
    // Configure I2C pins (base routine)
    //
    static uint8_t pinConfigure_(struct i2cStruct* i2c, uint8_t bus, i2c_pins pins, i2c_pullup pullup, uint8_t reconfig=1);
    //
    // Configure I2C pins - reconfigures active I2C pins on-the-fly (only works when bus is idle).  If reconfig
    //                      set then inactive pins will switch to input mode using same pullup configuration.
    // return: 1=success, 0=fail (bus busy)
    // parameters:
    //      pins = Wire: I2C_PINS_18_19, I2C_PINS_16_17 | Wire1(3.1): I2C_PINS_29_30, I2C_PINS_26_31 | Wire1(LC): I2C_PINS_22_23
    //      pullup = I2C_PULLUP_EXT, I2C_PULLUP_INT
    //      reconfig = 1=reconfigure alternate pins, 0=do not reconfigure alternate pins (base routine only)
    //
    inline uint8_t pinConfigure(i2c_pins pins, i2c_pullup pullup) { return pinConfigure_(i2c, bus, pins, pullup, 1); }

    // ------------------------------------------------------------------------------------------------------
    // Acquire Bus (static) - acquires bus in Master mode and escalates priority as needed, intended
    //                        for internal use only
    // return: 1=success, 0=fail (cannot acquire bus)
    // parameters:
    //      timeout = timeout in microseconds
    //      forceImm = flag to indicate if immediate mode is required
    //
    static uint8_t acquireBus_(struct i2cStruct* i2c, uint8_t bus, uint32_t timeout, uint8_t& forceImm);

    // ------------------------------------------------------------------------------------------------------
    // Set Default Timeout - sets the default timeout to be applied to all functions called with a timeout of
    //                       zero (the default in cases where timeout is not specified).  The default is
    //                       initially zero (infinite wait).
    // return: none
    // parameters:
    //      timeout = timeout in microseconds
    inline void setDefaultTimeout(uint32_t timeout) { i2c->defTimeout = timeout; }

    // ------------------------------------------------------------------------------------------------------
    // Reset Bus - toggles SCL until SDA line is released (9 clocks max).  This is used to correct
    //             a hung bus in which a Slave device missed some clocks and remains stuck outputting
    //             a low signal on SDA (thereby preventing START/STOP signaling).
    // return: none
    //
    static void resetBus_(struct i2cStruct* i2c, uint8_t bus);
    inline void resetBus(void) { resetBus_(i2c, bus); }

    // ------------------------------------------------------------------------------------------------------
    // Setup Master Transmit - initialize Tx buffer for transmit to slave at address
    // return: none
    // parameters:
    //      address = target 7bit slave address
    //
    void beginTransmission(uint8_t address);
    inline void beginTransmission(int address) { beginTransmission((uint8_t)address); }

    // ------------------------------------------------------------------------------------------------------
    // Master Transmit (base routine) - cannot be static due to call to getError() and in turn getWriteError()
    //
    uint8_t endTransmission(struct i2cStruct* i2c, uint8_t bus, i2c_stop sendStop, uint32_t timeout);
    //
    // Master Transmit - blocking routine with timeout, transmits Tx buffer to slave. i2c_stop parameter can be used
    //                   to indicate if command should end with a STOP(I2C_STOP) or not (I2C_NOSTOP).
    // return: 0=success, 1=data too long, 2=recv addr NACK, 3=recv data NACK, 4=other error
    // parameters:
    //      i2c_stop = I2C_NOSTOP, I2C_STOP
    //      timeout = timeout in microseconds
    //
    inline uint8_t endTransmission(i2c_stop sendStop, uint32_t timeout) { return endTransmission(i2c, bus, sendStop, timeout); }
    //
    // Master Transmit - blocking routine, transmits Tx buffer to slave
    // return: 0=success, 1=data too long, 2=recv addr NACK, 3=recv data NACK, 4=other error
    //
    inline uint8_t endTransmission(void) { return endTransmission(i2c, bus, I2C_STOP, 0); }
    //
    // Master Transmit - blocking routine, transmits Tx buffer to slave. i2c_stop parameter can be used to indicate
    //                   if command should end with a STOP (I2C_STOP) or not (I2C_NOSTOP).
    // return: 0=success, 1=data too long, 2=recv addr NACK, 3=recv data NACK, 4=other error
    // parameters:
    //      i2c_stop = I2C_NOSTOP, I2C_STOP
    //
    inline uint8_t endTransmission(i2c_stop sendStop) { return endTransmission(i2c, bus, sendStop, 0); }
    inline uint8_t endTransmission(uint8_t sendStop) { return endTransmission(i2c, bus, (i2c_stop)sendStop, 0); } // Wire compatibility

    // ------------------------------------------------------------------------------------------------------
    // Send Master Transmit (base routine)
    //
    static void sendTransmission_(struct i2cStruct* i2c, uint8_t bus, i2c_stop sendStop, uint32_t timeout);
    //
    // Send Master Transmit - non-blocking routine, starts transmit of Tx buffer to slave. Defaults to I2C_STOP.
    //                        Use done() or finish() to determine completion and status() to determine success/fail.
    // return: none
    //
    inline void sendTransmission(void) { sendTransmission_(i2c, bus, I2C_STOP, 0); }
    //
    // Send Master Transmit - non-blocking routine, starts transmit of Tx buffer to slave. i2c_stop parameter can be
    //                        used to indicate if command should end with a STOP (I2C_STOP) or not (I2C_NOSTOP). Use
    //                        done() or finish() to determine completion and status() to determine success/fail.
    // return: none
    // parameters:
    //      i2c_stop = I2C_NOSTOP, I2C_STOP
    //
    inline void sendTransmission(i2c_stop sendStop) { sendTransmission_(i2c, bus, sendStop, 0); }

    // ------------------------------------------------------------------------------------------------------
    // Master Receive (base routine)
    //
    static size_t requestFrom_(struct i2cStruct* i2c, uint8_t bus, uint8_t addr, size_t len, i2c_stop sendStop, uint32_t timeout);
    //
    // Master Receive - blocking routine, requests length bytes from slave at address. Receive data will be placed
    //                  in the Rx buffer.
    // return: #bytes received = success, 0=fail
    // parameters:
    //      address = target 7bit slave address
    //      length = number of bytes requested
    //
    inline size_t requestFrom(uint8_t addr, size_t len)
        { return requestFrom_(i2c, bus, addr, len, I2C_STOP, 0); }
    inline size_t requestFrom(int addr, int len)
        { return requestFrom_(i2c, bus, (uint8_t)addr, (size_t)len, I2C_STOP, 0); }
    inline uint8_t requestFrom(uint8_t addr, uint8_t len)
        { return (uint8_t)requestFrom_(i2c, bus, addr, (size_t)len, I2C_STOP, 0); } // Wire compatibility
    //
    // Master Receive - blocking routine, requests length bytes from slave at address. Receive data will be placed
    //                  in the Rx buffer. i2c_stop parameter can be used to indicate if command should end with a
    //                  STOP (I2C_STOP) or not (I2C_NOSTOP).
    // return: #bytes received = success, 0=fail
    // parameters:
    //      address = target 7bit slave address
    //      length = number of bytes requested
    //      i2c_stop = I2C_NOSTOP, I2C_STOP
    //
    inline size_t requestFrom(uint8_t addr, size_t len, i2c_stop sendStop)
        { return requestFrom_(i2c, bus, addr, len, sendStop, 0); }
    inline uint8_t requestFrom(uint8_t addr, uint8_t len, uint8_t sendStop)
        { return (uint8_t)requestFrom_(i2c, bus, addr, (size_t)len, (i2c_stop)sendStop, 0); } // Wire compatibility
    //
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
    inline size_t requestFrom(uint8_t addr, size_t len, i2c_stop sendStop, uint32_t timeout)
        { return requestFrom_(i2c, bus, addr, len, sendStop, timeout); }

    // ------------------------------------------------------------------------------------------------------
    // Start Master Receive (base routine)
    //
    static void sendRequest_(struct i2cStruct* i2c, uint8_t bus, uint8_t addr, size_t len, i2c_stop sendStop, uint32_t timeout);
    //
    // Start Master Receive - non-blocking routine, starts request for length bytes from slave at address. Receive
    //                        data will be placed in the Rx buffer. i2c_stop parameter can be used to indicate if
    //                        command should end with a STOP (I2C_STOP) or not (I2C_NOSTOP). Use done() or finish()
    //                        to determine completion and status() to determine success/fail.
    // return: none
    // parameters:
    //      address = target 7bit slave address
    //      length = number of bytes requested
    //      i2c_stop = I2C_NOSTOP, I2C_STOP
    //
    inline void sendRequest(uint8_t addr, size_t len, i2c_stop sendStop) { sendRequest_(i2c, bus, addr, len, sendStop, 0); }

    // ------------------------------------------------------------------------------------------------------
    // Get Wire Error - returns "Wire" error code from a failed Tx/Rx command
    // return: 0=success, 1=data too long, 2=recv addr NACK, 3=recv data NACK, 4=other error
    //
    uint8_t getError(void);

    // ------------------------------------------------------------------------------------------------------
    // Return Status - returns current status of I2C (enum return value)
    // return: I2C_WAITING, I2C_SENDING, I2C_SEND_ADDR, I2C_RECEIVING, I2C_TIMEOUT, I2C_ADDR_NAK, I2C_DATA_NAK,
    //         I2C_ARB_LOST, I2C_SLAVE_TX, I2C_SLAVE_RX
    //
    inline i2c_status status(void) { return i2c->currentStatus; }

    // ------------------------------------------------------------------------------------------------------
    // Return Status (base routine)
    //
    static uint8_t done_(struct i2cStruct* i2c);
    //
    // Done Check - returns simple complete/not-complete value to indicate I2C status
    // return: 1=Tx/Rx complete (with or without errors), 0=still running
    //
    inline uint8_t done(void) { return done_(i2c); }

    // ------------------------------------------------------------------------------------------------------
    // Return Status (base routine)
    //
    static uint8_t finish_(struct i2cStruct* i2c, uint8_t bus, uint32_t timeout);
    //
    // Finish - blocking routine, loops until Tx/Rx is complete
    // return: 1=success (Tx or Rx completed, no error), 0=fail (NAK, timeout or Arb Lost)
    //
    inline uint8_t finish(void) { return finish_(i2c, bus, 0); }
    //
    // Finish - blocking routine with timeout, loops until Tx/Rx is complete or timeout occurs
    // return: 1=success (Tx or Rx completed, no error), 0=fail (NAK, timeout or Arb Lost)
    // parameters:
    //      timeout = timeout in microseconds
    //
    inline uint8_t finish(uint32_t timeout) { return finish_(i2c, bus, timeout); }

    // ------------------------------------------------------------------------------------------------------
    // Write - write data byte to Tx buffer
    // return: #bytes written = success, 0=fail
    // parameters:
    //      data = data byte
    //
    size_t write(uint8_t data);
    inline size_t write(unsigned long n) { return write((uint8_t)n); }
    inline size_t write(long n)          { return write((uint8_t)n); }
    inline size_t write(unsigned int n)  { return write((uint8_t)n); }
    inline size_t write(int n)           { return write((uint8_t)n); }

    // ------------------------------------------------------------------------------------------------------
    // Write Array - write length number of bytes from data array to Tx buffer
    // return: #bytes written = success, 0=fail
    // parameters:
    //      data = pointer to uint8_t (or char) array of data
    //      length = number of bytes to write
    //
    size_t write(const uint8_t* data, size_t quantity);
    inline size_t write(const char* str) { return write((const uint8_t*)str, strlen(str)); }

    // ------------------------------------------------------------------------------------------------------
    // Available - returns number of remaining available bytes in Rx buffer
    // return: #bytes available
    //
    inline int available(void) { return i2c->rxBufferLength - i2c->rxBufferIndex; }

    // ------------------------------------------------------------------------------------------------------
    // Read (base routine)
    //
    static int read_(struct i2cStruct* i2c);
    //
    // Read - returns next data byte (signed int) from Rx buffer
    // return: data, -1 if buffer empty
    //
    inline int read(void) { return read_(i2c); }

    // ------------------------------------------------------------------------------------------------------
    // Peek (base routine)
    //
    static int peek_(struct i2cStruct* i2c);
    //
    // Peek - returns next data byte (signed int) from Rx buffer without removing it from Rx buffer
    // return: data, -1 if buffer empty
    //
    inline int peek(void) { return peek_(i2c); }

    // ------------------------------------------------------------------------------------------------------
    // Read Byte (base routine)
    //
    static uint8_t readByte_(struct i2cStruct* i2c);
    //
    // Read Byte - returns next data byte (uint8_t) from Rx buffer
    // return: data, 0 if buffer empty
    //
    inline uint8_t readByte(void) { return readByte_(i2c); }

    // ------------------------------------------------------------------------------------------------------
    // Peek Byte (base routine)
    //
    static uint8_t peekByte_(struct i2cStruct* i2c);
    //
    // Peek Byte - returns next data byte (uint8_t) from Rx buffer without removing it from Rx buffer
    // return: data, 0 if buffer empty
    //
    inline uint8_t peekByte(void) { return peekByte_(i2c); }

    // ------------------------------------------------------------------------------------------------------
    // Flush (not implemented)
    //
    inline void flush(void) {}

    // ------------------------------------------------------------------------------------------------------
    // Get Rx Address - returns target address of incoming I2C command. Used for Slaves operating over an address range.
    // return: rxAddr of last received command
    //
    inline uint8_t getRxAddr(void) { return i2c->rxAddr; }

    // ------------------------------------------------------------------------------------------------------
    // Set callback function for Slave Rx
    //
    inline void onReceive(void (*function)(size_t len)) { i2c->user_onReceive = function; }

    // ------------------------------------------------------------------------------------------------------
    // Set callback function for Slave Tx
    //
    inline void onRequest(void (*function)(void)) { i2c->user_onRequest = function; }

    // ------------------------------------------------------------------------------------------------------
    // For compatibility with pre-1.0 sketches and libraries
    inline void send(uint8_t b)             { write(b); }
    inline void send(uint8_t* s, uint8_t n) { write(s, n); }
    inline void send(int n)                 { write((uint8_t)n); }
    inline void send(char* s)               { write(s); }
    inline uint8_t receive(void)            { int c = read(); return (c<0) ? 0 : c; }

    // ------------------------------------------------------------------------------------------------------
    // Immediate operation
    static void i2c_wait_(struct i2cStruct* i2c) { while(!(*(i2c->S) & I2C_S_IICIF)){} *(i2c->S) = I2C_S_IICIF; }
};

extern i2c_t3 Wire;
#if I2C_BUS_NUM >= 2
    extern i2c_t3 Wire1;
#endif

#endif // I2C_T3_H
