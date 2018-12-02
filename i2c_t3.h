/*
    ------------------------------------------------------------------------------------------------------
    i2c_t3 - I2C library for Teensy 3.x & LC

    - (v11.0) Modified 01Dec18 by Brian (nox771 at gmail.com)

    Full changelog at end of file
    ------------------------------------------------------------------------------------------------------
    Copyright (c) 2013-2018, Brian (nox771 at gmail.com)

    Permission is hereby granted, free of charge, to any person obtaining a copy of this software and 
    associated documentation files (the "Software"), to deal in the Software without restriction, 
    including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
    and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, 
    subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all copies or substantial 
    portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT 
    LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION 
    WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
    ------------------------------------------------------------------------------------------------------
*/

#if defined(__AVR__)
#error "Sorry, i2c_t3 only works on Teensy LC and 3.x boards.  Use Wire for Teensy 2.0."
#endif

#if !defined(I2C_T3_H) && (defined(__MK20DX128__) || defined(__MK20DX256__) || defined(__MKL26Z64__) || \
                           defined(__MK64FX512__) || defined(__MK66FX1M0__)) // 3.0/3.1-3.2/LC/3.5/3.6
#define I2C_T3_H

#include <inttypes.h>
#include <stdio.h> // for size_t
#include "Arduino.h"
#include <DMAChannel.h>

// TODO missing kinetis.h defs
#ifndef I2C_F_DIV52
    #define I2C_F_DIV52  ((uint8_t)0x43)
    #define I2C_F_DIV60  ((uint8_t)0x45)
    #define I2C_F_DIV136 ((uint8_t)0x4F)
    #define I2C_F_DIV176 ((uint8_t)0x55)
    #define I2C_F_DIV352 ((uint8_t)0x95)
    #define I2C_FLT_SSIE    ((uint8_t)0x20)         // Start/Stop Interrupt Enable
    #define I2C_FLT_STARTF  ((uint8_t)0x10)         // Start Detect Flag
#endif


// ======================================================================================================
// == Start User Define Section =========================================================================
// ======================================================================================================

// ------------------------------------------------------------------------------------------------------
// I2C Bus Enable control - change to enable buses as follows.  The number of buses will be limited to the
//                          lower of this setting or what is available on the device.
//
// Teensy   Max #Buses
// ------   ----------
//   LC         2
//  3.0         1
//  3.1         2
//  3.2         2
//  3.5         3
//  3.6         4
//
// I2C_BUS_ENABLE 1   (enable Wire only)
// I2C_BUS_ENABLE 2   (enable Wire & Wire1)
// I2C_BUS_ENABLE 3   (enable Wire & Wire1 & Wire2)
// I2C_BUS_ENABLE 4   (enable Wire & Wire1 & Wire2 & Wire3)
//
#define I2C_BUS_ENABLE 4

// ------------------------------------------------------------------------------------------------------
// Tx/Rx buffer sizes - modify these as needed.  Buffers should be large enough to hold:
//                      Target Addr + Data payload.  Default is: 1byte Addr + 258byte Data
//                      (this can be substantially reduced if working with sensors or small data packets)
//
#define I2C_TX_BUFFER_LENGTH 259
#define I2C_RX_BUFFER_LENGTH 259

// ------------------------------------------------------------------------------------------------------
// Interrupt flag - uncomment and set below to make the specified pin high whenever the
//                  I2C interrupt occurs (modify pin number as needed).  This is useful as a
//                  trigger signal when using a logic analyzer.
//
//#define I2C0_INTR_FLAG_PIN 5
//#define I2C1_INTR_FLAG_PIN 6
//#define I2C2_INTR_FLAG_PIN 9
//#define I2C3_INTR_FLAG_PIN 10

// ------------------------------------------------------------------------------------------------------
// Auto retry - uncomment to make the library automatically call resetBus() if it has a timeout while
//              trying to send a START (occurs at the beginning of any endTransmission() or requestFrom()
//              call).  This will toggle SCL to try and get a hung Slave device to release the SDA line.
//              If successful then it will try again to send a START, if not then it will return a timeout
//              error (same as if auto retry was not defined).
//
// Note: this is incompatible with multi-master buses, only use in single-master configurations
//
//#define I2C_AUTO_RETRY

// ------------------------------------------------------------------------------------------------------
// Error counters - uncomment to make the library track error counts.  Error counts can be retrieved or
//                  zeroed using the getErrorCount() and zeroErrorCount() functions respectively.
//                  When included, errors will be tracked on the following (Master-mode only):
//                  Reset Bus (auto-retry only), Timeout, Addr NAK, Data NAK, Arb Lost, Bus Not Acquired,
//                  DMA Errors
//
#define I2C_ERROR_COUNTERS

// ------------------------------------------------------------------------------------------------------
// Disable priority check - uncomment this to entirely disable auto priority escalation.  Normally
//                          priority escalation occurs to ensure I2C ISR operates at a higher priority
//                          than the calling function (to prevent ISR stall if the calling function
//                          blocks).  Uncommenting this will disable the check and cause I2C ISR to
//                          remain at default priority.  It is recommended to disable this check and
//                          manually set ISR priority levels when using complex configurations.
//
//#define I2C_DISABLE_PRIORITY_CHECK


// ======================================================================================================
// == End User Define Section ===========================================================================
// ======================================================================================================


// ------------------------------------------------------------------------------------------------------
// Set number of enabled buses
//
#if defined(__MK20DX128__) // 3.0
    #define I2C_BUS_NUM 1
#elif (defined(__MK20DX256__) || defined(__MKL26Z64__)) && (I2C_BUS_ENABLE >= 2) // 3.1/3.2/LC
    #define I2C_BUS_NUM 2
#elif defined(__MK64FX512__) && (I2C_BUS_ENABLE >= 3) // 3.5
    #define I2C_BUS_NUM 3
#elif defined(__MK66FX1M0__) && (I2C_BUS_ENABLE >= 4) // 3.6
    #define I2C_BUS_NUM 4
#else
    #define I2C_BUS_NUM I2C_BUS_ENABLE
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

#if defined(I2C2_INTR_FLAG_PIN)
    #define I2C2_INTR_FLAG_INIT do             \
    {                                          \
        pinMode(I2C2_INTR_FLAG_PIN, OUTPUT);   \
        digitalWrite(I2C2_INTR_FLAG_PIN, LOW); \
    } while(0)

    #define I2C2_INTR_FLAG_ON   do {digitalWrite(I2C2_INTR_FLAG_PIN, HIGH);} while(0)
    #define I2C2_INTR_FLAG_OFF  do {digitalWrite(I2C2_INTR_FLAG_PIN, LOW);} while(0)
#else
    #define I2C2_INTR_FLAG_INIT do{}while(0)
    #define I2C2_INTR_FLAG_ON   do{}while(0)
    #define I2C2_INTR_FLAG_OFF  do{}while(0)
#endif

#if defined(I2C3_INTR_FLAG_PIN)
    #define I2C3_INTR_FLAG_INIT do             \
    {                                          \
        pinMode(I2C3_INTR_FLAG_PIN, OUTPUT);   \
        digitalWrite(I2C3_INTR_FLAG_PIN, LOW); \
    } while(0)

    #define I2C3_INTR_FLAG_ON   do {digitalWrite(I2C3_INTR_FLAG_PIN, HIGH);} while(0)
    #define I2C3_INTR_FLAG_OFF  do {digitalWrite(I2C3_INTR_FLAG_PIN, LOW);} while(0)
#else
    #define I2C3_INTR_FLAG_INIT do{}while(0)
    #define I2C3_INTR_FLAG_ON   do{}while(0)
    #define I2C3_INTR_FLAG_OFF  do{}while(0)
#endif


// ------------------------------------------------------------------------------------------------------
// Error counters setup
//
#if defined(I2C_ERROR_COUNTERS)
    #define I2C_ERR_INC(i2c_err_count) do {if(i2c->errCounts[i2c_err_count] < UINT32_MAX) i2c->errCounts[i2c_err_count]++;} while(0)
#else
    #define I2C_ERR_INC(i2c_err_count) do{}while(0)
#endif


// ------------------------------------------------------------------------------------------------------
// Function argument enums
//
enum i2c_op_mode  {I2C_OP_MODE_IMM, I2C_OP_MODE_ISR, I2C_OP_MODE_DMA};
enum i2c_mode     {I2C_MASTER, I2C_SLAVE};
enum i2c_pullup   {I2C_PULLUP_EXT, I2C_PULLUP_INT};
enum i2c_rate     {I2C_RATE_100  = 100000,
                   I2C_RATE_200  = 200000,
                   I2C_RATE_300  = 300000,
                   I2C_RATE_400  = 400000,
                   I2C_RATE_600  = 600000,
                   I2C_RATE_800  = 800000,
                   I2C_RATE_1000 = 1000000,
                   I2C_RATE_1200 = 1200000,
                   I2C_RATE_1500 = 1500000,
                   I2C_RATE_1800 = 1800000,
                   I2C_RATE_2000 = 2000000,
                   I2C_RATE_2400 = 2400000,
                   I2C_RATE_2800 = 2800000,
                   I2C_RATE_3000 = 3000000};
enum i2c_stop     {I2C_NOSTOP, I2C_STOP};
enum i2c_status   {I2C_WAITING,     // stopped states
                   I2C_TIMEOUT,     //  |
                   I2C_ADDR_NAK,    //  |
                   I2C_DATA_NAK,    //  |
                   I2C_ARB_LOST,    //  |
                   I2C_BUF_OVF,     //  |
                   I2C_NOT_ACQ,     //  |
                   I2C_DMA_ERR,     //  V
                   I2C_SENDING,     // active states
                   I2C_SEND_ADDR,   //  |
                   I2C_RECEIVING,   //  |
                   I2C_SLAVE_TX,    //  |
                   I2C_SLAVE_RX};   //  V
enum i2c_dma_state {I2C_DMA_OFF,
                    I2C_DMA_ADDR,
                    I2C_DMA_BULK,
                    I2C_DMA_LAST};
#if defined(__MKL26Z64__) // LC
    enum i2c_pins {I2C_PINS_16_17 = 0,      // 16 SCL0  17 SDA0
                   I2C_PINS_18_19,          // 19 SCL0  18 SDA0
                   I2C_PINS_22_23,          // 22 SCL1  23 SDA1
                   I2C_PINS_DEFAULT,
                   I2C_PINS_COUNT};
    const uint8_t i2c_valid_pins[] = { 0, 16, 17, 2,  // bus, scl, sda, alt
                                       0, 19, 18, 2,
                                       1, 22, 23, 2,
                                       0,  0,  0, 0 };
#elif defined(__MK20DX128__) // 3.0
    enum i2c_pins {I2C_PINS_16_17 = 0,      // 16 SCL0  17 SDA0
                   I2C_PINS_18_19,          // 19 SCL0  18 SDA0
                   I2C_PINS_DEFAULT,
                   I2C_PINS_COUNT};
    const uint8_t i2c_valid_pins[] = { 0, 16, 17, 2,  // bus, scl, sda, alt
                                       0, 19, 18, 2,
                                       0,  0,  0, 0 };
#elif defined(__MK20DX256__) // 3.1/3.2
    enum i2c_pins {I2C_PINS_16_17 = 0,      // 16 SCL0  17 SDA0
                   I2C_PINS_18_19,          // 19 SCL0  18 SDA0
                   I2C_PINS_29_30,          // 29 SCL1  30 SDA1
                   I2C_PINS_26_31,          // 26 SCL1  31 SDA1
                   I2C_PINS_DEFAULT,
                   I2C_PINS_COUNT};
    const uint8_t i2c_valid_pins[] = { 0, 16, 17, 2,  // bus, scl, sda, alt
                                       0, 19, 18, 2,
                                       1, 29, 30, 2,
                                       1, 26, 31, 6,
                                       0,  0,  0, 0 };
#elif defined(__MK64FX512__) // 3.5
    enum i2c_pins {I2C_PINS_3_4 = 0,        //  3 SCL2   4 SDA2
                   I2C_PINS_7_8,            //  7 SCL0   8 SDA0
                   I2C_PINS_16_17,          // 16 SCL0  17 SDA0
                   I2C_PINS_18_19,          // 19 SCL0  18 SDA0
                   I2C_PINS_33_34,          // 33 SCL0  34 SDA0
                   I2C_PINS_37_38,          // 37 SCL1  38 SDA1
                   I2C_PINS_47_48,          // 47 SCL0  48 SDA0
                   I2C_PINS_DEFAULT,
                   I2C_PINS_COUNT};
    const uint8_t i2c_valid_pins[] = { 2,  3,  4, 5,  // bus, scl, sda, alt
                                       0,  7,  8, 7,
                                       0, 16, 17, 2,
                                       0, 19, 18, 2,
                                       0, 33, 34, 5,
                                       1, 37, 38, 2,
                                       0, 47, 48, 2,
                                       0,  0,  0, 0 };
#elif defined(__MK66FX1M0__) // 3.6
    enum i2c_pins {I2C_PINS_3_4 = 0,        //  3 SCL2   4 SDA2
                   I2C_PINS_7_8,            //  7 SCL0   8 SDA0
                   I2C_PINS_16_17,          // 16 SCL0  17 SDA0
                   I2C_PINS_18_19,          // 19 SCL0  18 SDA0
                   I2C_PINS_33_34,          // 33 SCL0  34 SDA0
                   I2C_PINS_37_38,          // 37 SCL1  38 SDA1
                   I2C_PINS_47_48,          // 47 SCL0  48 SDA0
                   I2C_PINS_56_57,          // 57 SCL3  56 SDA3
                   I2C_PINS_DEFAULT,
                   I2C_PINS_COUNT};
    const uint8_t i2c_valid_pins[] = { 2,  3,  4, 5,  // bus, scl, sda, alt
                                       0,  7,  8, 7,
                                       0, 16, 17, 2,
                                       0, 19, 18, 2,
                                       0, 33, 34, 5,
                                       1, 37, 38, 2,
                                       0, 47, 48, 2,
                                       3, 57, 56, 2,
                                       0,  0,  0, 0 };
#endif
enum i2c_err_count {I2C_ERRCNT_RESET_BUS=0,
                    I2C_ERRCNT_TIMEOUT,
                    I2C_ERRCNT_ADDR_NAK,
                    I2C_ERRCNT_DATA_NAK,
                    I2C_ERRCNT_ARBL,
                    I2C_ERRCNT_NOT_ACQ,
                    I2C_ERRCNT_DMA_ERR};


// ------------------------------------------------------------------------------------------------------
// Divide ratio tables
//
const int32_t i2c_div_num[] =
    {20,22,24,26,28,30,32,34,36,40,44,48,52,56,60,64,68,72,
     80,88,96,104,112,128,136,144,160,176,192,224,240,256,288,
     320,352,384,448,480,512,576,640,768,896,960,1024,1152,
     1280,1536,1920,1792,2048,2304,2560,3072,3840};

const uint8_t i2c_div_ratio[] =
    {I2C_F_DIV20,I2C_F_DIV22,I2C_F_DIV24,I2C_F_DIV26,
     I2C_F_DIV28,I2C_F_DIV30,I2C_F_DIV32,I2C_F_DIV34,
     I2C_F_DIV36,I2C_F_DIV40,I2C_F_DIV44,I2C_F_DIV48,
     I2C_F_DIV52,I2C_F_DIV56,I2C_F_DIV60,I2C_F_DIV64,
     I2C_F_DIV68,I2C_F_DIV72,I2C_F_DIV80,I2C_F_DIV88,
     I2C_F_DIV96,I2C_F_DIV104,I2C_F_DIV112,I2C_F_DIV128,
     I2C_F_DIV136,I2C_F_DIV144,I2C_F_DIV160,I2C_F_DIV176,
     I2C_F_DIV192,I2C_F_DIV224,I2C_F_DIV240,I2C_F_DIV256,
     I2C_F_DIV288,I2C_F_DIV320,I2C_F_DIV352,I2C_F_DIV384,
     I2C_F_DIV448,I2C_F_DIV480,I2C_F_DIV512,I2C_F_DIV576,
     I2C_F_DIV640,I2C_F_DIV768,I2C_F_DIV896,I2C_F_DIV960,
     I2C_F_DIV1024,I2C_F_DIV1152,I2C_F_DIV1280,I2C_F_DIV1536,
     I2C_F_DIV1920,I2C_F_DIV1792,I2C_F_DIV2048,I2C_F_DIV2304,
     I2C_F_DIV2560,I2C_F_DIV3072,I2C_F_DIV3840};


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
    volatile uint8_t  currentSCL;            // Current SCL pin                   (User&ISR)
    volatile uint8_t  currentSDA;            // Current SDA pin                   (User&ISR)
    i2c_pullup currentPullup;                // Current Pullup                    (User)
    uint32_t currentRate;                    // Current Rate                      (User)
    i2c_stop currentStop;                    // Current Stop                      (User)
    volatile i2c_status currentStatus;       // Current Status                    (User&ISR)
    uint8_t  rxAddr;                         // Rx Address                        (ISR)
    size_t   reqCount;                       // Byte Request Count                (User)
    uint8_t  irqCount;                       // IRQ Count, used by SDA-rising ISR (ISR)
    uint8_t  timeoutRxNAK;                   // Rx Timeout NAK flag               (ISR)
    volatile i2c_dma_state activeDMA;        // Active DMA flag                   (User&ISR)
    void (*user_onTransmitDone)(void);       // Master Tx Callback Function       (User)
    void (*user_onReqFromDone)(void);        // Master Rx Callback Function       (User)
    void (*user_onReceive)(size_t len);      // Slave Rx Callback Function        (User)
    void (*user_onRequest)(void);            // Slave Tx Callback Function        (User)
    void (*user_onError)(void);              // Error Callback Function           (User)
    DMAChannel* DMA;                         // DMA Channel object                (User&ISR)
    uint32_t defTimeout;                     // Default Timeout                   (User)
    volatile uint32_t errCounts[7];          // Error Counts Array                (User&ISR)
    uint8_t  configuredSCL;                  // SCL configured flag               (User)
    uint8_t  configuredSDA;                  // SDA configured flag               (User)
};


// ------------------------------------------------------------------------------------------------------
// I2C Class
//
extern "C" void i2c0_isr(void);
#if I2C_BUS_NUM >= 2
    extern "C" void i2c1_isr(void);
#endif
#if I2C_BUS_NUM >= 3
    extern "C" void i2c2_isr(void);
#endif
#if I2C_BUS_NUM >= 4
    extern "C" void i2c3_isr(void);
#endif
extern "C" void i2c_isr_handler(struct i2cStruct* i2c, uint8_t bus);

class i2c_t3 : public Stream
{
private:
    //
    // I2C data structures - these need to be static so "C" ISRs can use them
    //
    static struct i2cStruct i2cData[I2C_BUS_NUM];
    //
    // Base handler - ISRs are non-class global functions, friend of class req'd for data access
    //
    friend void i2c_isr_handler(struct i2cStruct* i2c, uint8_t bus);
    //
    // Bus ISRs
    //
    friend void i2c0_isr(void);                 // I2C0 ISR
    #if (defined(__MK20DX128__) || defined(__MK20DX256__))
        static void sda_rising_isr_handler(struct i2cStruct* i2c, uint8_t bus); // Slave STOP base handler - 3.0/3.1/3.2 only
        static void sda0_rising_isr(void);      // Slave STOP detection (I2C0) - 3.0/3.1/3.2 only
    #endif
    #if I2C_BUS_NUM >= 2
        friend void i2c1_isr(void);             // I2C1 ISR
        #if defined(__MK20DX256__)
            static void sda1_rising_isr(void);  // Slave STOP detection (I2C1) - 3.1/3.2 only
        #endif
    #endif
    #if I2C_BUS_NUM >= 3
        friend void i2c2_isr(void);             // I2C2 ISR
    #endif
    #if I2C_BUS_NUM >= 4
        friend void i2c3_isr(void);             // I2C3 ISR
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
    //
    // I2C ISR Active flag - this is used to disable priority escalation.  Increment to 1 to disable priority
    //                       check.  This is a global flag to prevent complex cross-bus issues.  It is only
    //                       incremented/decremented, not set.
    //
    static volatile uint8_t isrActive;

    // ------------------------------------------------------------------------------------------------------
    // Constructor
    //
    i2c_t3(uint8_t i2c_bus);
    ~i2c_t3();

    // ------------------------------------------------------------------------------------------------------
    // Pin Mapping - convert i2c_pins enum into pin values, intended for internal use only
    //
    inline uint8_t mapSCL(i2c_pins pins) { return i2c_valid_pins[pins*4+1]; }
    inline uint8_t mapSDA(i2c_pins pins) { return i2c_valid_pins[pins*4+2]; }

    // ------------------------------------------------------------------------------------------------------
    // Valid pin checks - verify if SCL or SDA pin is valid, intended for internal use only
    // return: alt setting, 0=not valid
    // parameters:
    //      bus = bus number
    //      pin = pin number to check
    //      offset = array offset
    //
    static uint8_t validPin_(uint8_t bus, uint8_t pin, uint8_t offset);

    // ------------------------------------------------------------------------------------------------------
    // Initialize I2C (base routine)
    //
    static void begin_(struct i2cStruct* i2c, uint8_t bus, i2c_mode mode, uint8_t address1, uint8_t address2,
                       uint8_t pinSCL, uint8_t pinSDA, i2c_pullup pullup, uint32_t rate, i2c_op_mode opMode);
    //
    // Initialize I2C (Master) - initializes I2C as Master mode, external pullups, 100kHz rate,
    //                           and default pin setting
    // default pin setting SCL/SDA:
    //      Wire:   19/18
    //      Wire1:  29/30 (3.1/3.2), 22/23 (LC), 37/38 (3.5/3.6)
    //      Wire2:   3/4  (3.5/3.6)
    //      Wire3:  57/56 (3.6)
    // return: none
    //
    inline void begin(void)
        { begin_(i2c, bus, I2C_MASTER, 0, 0, 0, 0, I2C_PULLUP_EXT, 100000, I2C_OP_MODE_ISR); }
    //
    // Initialize I2C (Slave) - initializes I2C as Slave mode using address, external pullups, 100kHz rate,
    //                          and default pin setting
    // default pin setting SCL/SDA:
    //      Wire:   19/18
    //      Wire1:  29/30 (3.1/3.2), 22/23 (LC), 37/38 (3.5/3.6)
    //      Wire2:   3/4  (3.5/3.6)
    //      Wire3:  57/56 (3.6)
    // return: none
    // parameters:
    //      address = 7bit slave address of device
    //
    inline void begin(int address)
        { begin_(i2c, bus, I2C_SLAVE, (uint8_t)address, 0, 0, 0, I2C_PULLUP_EXT, 100000, I2C_OP_MODE_ISR); }
    inline void begin(uint8_t address)
        { begin_(i2c, bus, I2C_SLAVE, address, 0, 0, 0, I2C_PULLUP_EXT, 100000, I2C_OP_MODE_ISR); }
    //
    // Initialize I2C - initializes I2C as Master or single address Slave
    // return: none
    // parameters (optional parameters marked '^'):
    //      mode = I2C_MASTER, I2C_SLAVE
    //      address1 = 7bit slave address when configured as Slave (ignored for Master mode)
    //     ^address2 = 2nd 7bit address for specifying Slave address range (ignored for Master mode)
    //     ^pins = pins to use, can be specified as 'i2c_pins' enum,
    //             or as 'SCL,SDA' pair (using any valid SCL or SDA), options are:
    //                                 Pin Name
    //          Interface  Devices   (deprecated)    SCL    SDA   Default
    //          ---------  -------  --------------  -----  -----  -------
    //             Wire      All    I2C_PINS_16_17    16     17
    //             Wire      All    I2C_PINS_18_19    19*    18      +
    //             Wire    3.5/3.6  I2C_PINS_7_8       7      8
    //             Wire    3.5/3.6  I2C_PINS_33_34    33     34
    //             Wire    3.5/3.6  I2C_PINS_47_48    47     48
    //            Wire1       LC    I2C_PINS_22_23    22     23      +
    //            Wire1    3.1/3.2  I2C_PINS_26_31    26     31
    //            Wire1    3.1/3.2  I2C_PINS_29_30    29     30      +
    //            Wire1    3.5/3.6  I2C_PINS_37_38    37     38      +
    //            Wire2    3.5/3.6  I2C_PINS_3_4       3      4      +
    //            Wire3      3.6    I2C_PINS_56_57    57*    56      +
    //          (note: in almost all cases SCL is the lower pin #, except cases marked *)
    //     ^pullup = I2C_PULLUP_EXT, I2C_PULLUP_INT (default I2C_PULLUP_EXT)
    //     ^rate = I2C frequency to use, can be specified directly in Hz, eg. 400000 for 400kHz, or using one of the
    //             following enum values (deprecated):
    //             I2C_RATE_100, I2C_RATE_200, I2C_RATE_300, I2C_RATE_400,
    //             I2C_RATE_600, I2C_RATE_800, I2C_RATE_1000, I2C_RATE_1200,
    //             I2C_RATE_1500, I2C_RATE_1800, I2C_RATE_2000, I2C_RATE_2400,
    //             I2C_RATE_2800, I2C_RATE_3000
    //     ^opMode = I2C_OP_MODE_IMM, I2C_OP_MODE_ISR, I2C_OP_MODE_DMA (ignored for Slave mode, defaults ISR mode)
    //
    inline void begin(i2c_mode mode, uint8_t address1, i2c_pins pins=I2C_PINS_DEFAULT,
                      i2c_pullup pullup=I2C_PULLUP_EXT, uint32_t rate=400000, i2c_op_mode opMode=I2C_OP_MODE_ISR)
        { begin_(i2c, bus, mode, address1, 0, mapSCL(pins), mapSDA(pins), pullup, rate, opMode); }

    inline void begin(i2c_mode mode, uint8_t address1, uint8_t pinSCL, uint8_t pinSDA,
                      i2c_pullup pullup=I2C_PULLUP_EXT, uint32_t rate=400000, i2c_op_mode opMode=I2C_OP_MODE_ISR)
        { begin_(i2c, bus, mode, address1, 0, pinSCL, pinSDA, pullup, rate, opMode); }

    inline void begin(i2c_mode mode, uint8_t address1, uint8_t address2, i2c_pins pins=I2C_PINS_DEFAULT,
                      i2c_pullup pullup=I2C_PULLUP_EXT, uint32_t rate=400000, i2c_op_mode opMode=I2C_OP_MODE_ISR)
        { begin_(i2c, bus, mode, address1, address2, mapSCL(pins), mapSDA(pins), pullup, rate, opMode); }

    inline void begin(i2c_mode mode, uint8_t address1, uint8_t address2, uint8_t pinSCL, uint8_t pinSDA,
                      i2c_pullup pullup=I2C_PULLUP_EXT, uint32_t rate=400000, i2c_op_mode opMode=I2C_OP_MODE_ISR)
        { begin_(i2c, bus, mode, address1, address2, pinSCL, pinSDA, pullup, rate, opMode); }

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
    // Set I2C rate (base routine)
    //
    static void setRate_(struct i2cStruct* i2c, uint32_t busFreq, uint32_t i2cFreq);
    //
    // Set I2C rate - reconfigures I2C frequency divider based on supplied bus freq and desired I2C freq.
    //                This will be done assuming an idealized I2C rate, even though at high I2C rates
    //                the actual throughput is much lower than theoretical value.
    //
    //                Since the division ratios are quantized with non-uniform spacing, the selected rate
    //                will be the one using the nearest available divider.
    // return: none
    // parameters:
    //      busFreq = bus frequency, typically F_BUS unless reconfigured
    //      freq = desired I2C frequency (will be quantized to nearest rate), or can be I2C_RATE_XXX enum (deprecated),
    //             such as I2C_RATE_100, I2C_RATE_400, etc...
    //
    // Max I2C rate is 1/20th F_BUS.  Some examples:
    //
    //     F_CPU      F_BUS     Max I2C
    //     (MHz)      (MHz)       Rate
    // -------------  -----    ----------
    //    240/120      120        6.0M    bus overclock
    //      216        108        5.4M    bus overclock
    //     192/96       96        4.8M    bus overclock
    //      180         90        4.5M    bus overclock
    //      240         80        4.0M    bus overclock
    //   216/144/72     72        3.6M    bus overclock
    //      192         64        3.2M    bus overclock
    //  240/180/120     60        3.0M
    //      168         56        2.8M
    //      216         54        2.7M
    // 192/144/96/48    48        2.4M
    //       72         36        1.8M
    //       24         24        1.2M
    //       16         16        800k
    //        8          8        400k
    //        4          4        200k
    //        2          2        100k
    //
    inline void setRate(uint32_t busFreq, uint32_t i2cFreq) { setRate_(i2c, busFreq, i2cFreq); }
    // return value and i2c_rate enum no longer needed (deprecated form, always returns 1)
    inline uint8_t setRate(uint32_t busFreq, i2c_rate rate) { setRate_(i2c, busFreq, (uint32_t)rate); return 1; }
    // Wire compatibility
    inline void setClock(uint32_t i2cFreq)
    {
        #if defined(__MKL26Z64__) // LC
            if(i2c->currentSCL == 22)
                setRate_(i2c, (uint32_t)F_CPU, i2cFreq); // LC Wire1 bus uses system clock (F_CPU) instead of bus clock (F_BUS)
            else
                setRate_(i2c, (uint32_t)F_BUS, i2cFreq);
        #else
            setRate_(i2c, (uint32_t)F_BUS, i2cFreq);
        #endif
    }
    // i2c_t3 version of setClock (deprecated)
    inline uint8_t setRate(i2c_rate rate) { setClock((uint32_t)rate); return 1; }

    // ------------------------------------------------------------------------------------------------------
    // Get I2C clock - return current clock setting
    // return: uint32_t = bus frequency (Hz)
    // parameters: none
    inline uint32_t getClock(void) { return i2c->currentRate; }

    // ------------------------------------------------------------------------------------------------------
    // Configure I2C pins (base routine)
    //
    static uint8_t pinConfigure_(struct i2cStruct* i2c, uint8_t bus, uint8_t pinSCL, uint8_t pinSDA, i2c_pullup pullup, 
                                 uint8_t configuredSCL, uint8_t configuredSDA);
    //
    // ------------------------------------------------------------------------------------------------------
    // Configure I2C pins - reconfigures active I2C pins on-the-fly (only works when bus is idle).  If reconfig
    //                      set then inactive pins will switch to input mode using same pullup configuration.
    // return: 1=success, 0=fail (bus busy or incompatible pins)
    // parameters:
    //      pins = pins to use, can be specified as 'i2c_pins' enum,
    //             or as 'SCL,SDA' pair (using any valid SCL or SDA), options are:
    //                                 Pin Name
    //          Interface  Devices   (deprecated)    SCL    SDA   Default
    //          ---------  -------  --------------  -----  -----  -------
    //             Wire      All    I2C_PINS_16_17    16     17
    //             Wire      All    I2C_PINS_18_19    19*    18      +
    //             Wire    3.5/3.6  I2C_PINS_7_8       7      8
    //             Wire    3.5/3.6  I2C_PINS_33_34    33     34
    //             Wire    3.5/3.6  I2C_PINS_47_48    47     48
    //            Wire1       LC    I2C_PINS_22_23    22     23      +
    //            Wire1    3.1/3.2  I2C_PINS_26_31    26     31
    //            Wire1    3.1/3.2  I2C_PINS_29_30    29     30      +
    //            Wire1    3.5/3.6  I2C_PINS_37_38    37     38      +
    //            Wire2    3.5/3.6  I2C_PINS_3_4       3      4      +
    //            Wire3      3.6    I2C_PINS_56_57    57*    56      +
    //          (note: in almost all cases SCL is the lower pin #, except cases marked *)
    //      ^pullup = I2C_PULLUP_EXT, I2C_PULLUP_INT (default I2C_PULLUP_EXT)
    //
    inline uint8_t pinConfigure(i2c_pins pins, i2c_pullup pullup=I2C_PULLUP_EXT)
        { return pinConfigure_(i2c, bus, mapSCL(pins), mapSDA(pins), pullup, i2c->configuredSCL, i2c->configuredSDA); }
    inline uint8_t pinConfigure(uint8_t pinSCL, uint8_t pinSDA, i2c_pullup pullup=I2C_PULLUP_EXT)
        { return pinConfigure_(i2c, bus, pinSCL, pinSDA, pullup, i2c->configuredSCL, i2c->configuredSDA); }
    //
    // ------------------------------------------------------------------------------------------------------
    // Set SCL/SDA - change the SCL or SDA pin
    // return: none
    // parameters:
    //      pin = pin to use, if invalid then no change will occur
    inline void setSCL(uint8_t pin) 
        { pinConfigure_(i2c, bus, pin, i2c->currentSDA, i2c->currentPullup, i2c->configuredSCL, 1); }
    inline void setSDA(uint8_t pin) 
        { pinConfigure_(i2c, bus, i2c->currentSCL, pin, i2c->currentPullup, 1, i2c->configuredSDA); }
    //
    // ------------------------------------------------------------------------------------------------------
    // Get SCL/SDA - get the current SCL or SDA pin
    // return: pin used
    //
    inline uint8_t getSCL(void) { return i2c->currentSCL; }
    inline uint8_t getSDA(void) { return i2c->currentSDA; }

    // ------------------------------------------------------------------------------------------------------
    // Set Default Timeout - sets the default timeout to be applied to all functions called with a timeout of
    //                       zero (the default in cases where timeout is not specified).  The default is
    //                       initially zero (infinite wait).
    // return: none
    // parameters:
    //      timeout = timeout in microseconds
    inline void setDefaultTimeout(uint32_t timeout) { i2c->defTimeout = timeout; }

    // ------------------------------------------------------------------------------------------------------
    // Acquire Bus - acquires bus in Master mode and escalates priority as needed, intended
    //               for internal use only
    // return: 1=success, 0=fail (cannot acquire bus)
    // parameters:
    //      timeout = timeout in microseconds
    //      forceImm = flag to indicate if immediate mode is required
    //
    static uint8_t acquireBus_(struct i2cStruct* i2c, uint8_t bus, uint32_t timeout, uint8_t& forceImm);

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
    inline void beginTransmission(int address) { beginTransmission((uint8_t)address); } // Wire compatibility

    // ------------------------------------------------------------------------------------------------------
    // Master Transmit (base routine) - cannot be static due to call to getError() and in turn getWriteError()
    //
    uint8_t endTransmission(struct i2cStruct* i2c, uint8_t bus, i2c_stop sendStop, uint32_t timeout);
    //
    // Master Transmit - blocking routine, transmits Tx buffer to slave. i2c_stop parameter can be used
    //                   to indicate if command should end with a STOP(I2C_STOP) or not (I2C_NOSTOP).
    //                   Timeout parameter can be optionally specified.
    // return: 0=success, 1=data too long, 2=recv addr NACK, 3=recv data NACK, 4=other error
    // parameters:
    //      ^i2c_stop = I2C_NOSTOP, I2C_STOP (default STOP)
    //      ^timeout = timeout in microseconds (default 0 = infinite wait)
    //
    inline uint8_t endTransmission(i2c_stop sendStop=I2C_STOP, uint32_t timeout=0) { return endTransmission(i2c, bus, sendStop, timeout); }
    inline uint8_t endTransmission(uint8_t sendStop) { return endTransmission(i2c, bus, (i2c_stop)sendStop, 0); } // Wire compatibility

    // ------------------------------------------------------------------------------------------------------
    // Send Master Transmit (base routine)
    //
    static void sendTransmission_(struct i2cStruct* i2c, uint8_t bus, i2c_stop sendStop, uint32_t timeout);
    //
    // Send Master Transmit - non-blocking routine, starts transmit of Tx buffer to slave. i2c_stop parameter can be
    //                        used to indicate if command should end with a STOP (I2C_STOP) or not (I2C_NOSTOP). Use
    //                        done(), finish(), or onTransmitDone() callback to determine completion and
    //                        status() to determine success/fail.  Note that sendTransmission() does not currently
    //                        support timeouts (aside from initial bus acquisition which does support it).
    // return: none
    // parameters:
    //     ^i2c_stop = I2C_NOSTOP, I2C_STOP (default STOP)
    //
    inline void sendTransmission(i2c_stop sendStop=I2C_STOP) { sendTransmission_(i2c, bus, sendStop, 0); }

    // ------------------------------------------------------------------------------------------------------
    // Master Receive (base routine)
    //
    static size_t requestFrom_(struct i2cStruct* i2c, uint8_t bus, uint8_t addr, size_t len, i2c_stop sendStop, uint32_t timeout);
    //
    // Master Receive - Requests length bytes from slave at address. Receive data will be placed in the Rx buffer.
    //                  i2c_stop parameter can be used to indicate if command should end with a STOP (I2C_STOP) or
    //                  not (I2C_NOSTOP).  Timeout parameter can be optionally specified.
    // return: #bytes received = success, 0=fail (0 length request, NAK, timeout, or bus error)
    // parameters:
    //      address = target 7bit slave address
    //      length = number of bytes requested
    //     ^i2c_stop = I2C_NOSTOP, I2C_STOP (default STOP)
    //     ^timeout = timeout in microseconds (default 0 = infinite wait)
    //
    inline size_t requestFrom(uint8_t addr, size_t len, i2c_stop sendStop=I2C_STOP, uint32_t timeout=0)
        { return requestFrom_(i2c, bus, addr, len, sendStop, timeout); }
    inline size_t requestFrom(int addr, int len)
        { return requestFrom_(i2c, bus, (uint8_t)addr, (size_t)len, I2C_STOP, 0); } // Wire compatibility
    inline uint8_t requestFrom(uint8_t addr, uint8_t len, uint8_t sendStop=1)
        { return (uint8_t)requestFrom_(i2c, bus, addr, (size_t)len, (i2c_stop)sendStop, 0); } // Wire compatibility

    // ------------------------------------------------------------------------------------------------------
    // Start Master Receive (base routine)
    //
    static void sendRequest_(struct i2cStruct* i2c, uint8_t bus, uint8_t addr, size_t len, i2c_stop sendStop, uint32_t timeout);
    //
    // Start Master Receive - non-blocking routine, starts request for length bytes from slave at address. Receive
    //                        data will be placed in the Rx buffer. i2c_stop parameter can be used to indicate if
    //                        command should end with a STOP (I2C_STOP) or not (I2C_NOSTOP). Use done(), finish()
    //                        or onReqFromDone() callback to determine completion and status() to determine
    //                        success/fail.
    // return: none
    // parameters:
    //      address = target 7bit slave address
    //      length = number of bytes requested
    //     ^i2c_stop = I2C_NOSTOP, I2C_STOP (default STOP)
    //
    inline void sendRequest(uint8_t addr, size_t len, i2c_stop sendStop=I2C_STOP) { sendRequest_(i2c, bus, addr, len, sendStop, 0); }

    // ------------------------------------------------------------------------------------------------------
    // Get Wire Error - returns "Wire" error code from a failed Tx/Rx command
    // return: 0=success, 1=data too long, 2=recv addr NACK, 3=recv data NACK, 4=other error
    //
    uint8_t getError(void);

    // ------------------------------------------------------------------------------------------------------
    // Return Status - returns current status of I2C (enum return value)
    // return: I2C_WAITING, I2C_TIMEOUT, I2C_ADDR_NAK, I2C_DATA_NAK, I2C_ARB_LOST, I2C_BUF_OVF,
    //         I2C_NOT_ACQ, I2C_DMA_ERR, I2C_SENDING, I2C_SEND_ADDR, I2C_RECEIVING, I2C_SLAVE_TX, I2C_SLAVE_RX
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
    // Finish - blocking routine, loops until Tx/Rx is complete.  Timeout parameter can be optionally specified.
    // return: 1=success (Tx or Rx completed, no error), 0=fail (NAK, timeout or Arb Lost)
    // parameters:
    //      ^timeout = timeout in microseconds (default 0 = infinite wait)
    //
    inline uint8_t finish(uint32_t timeout=0) { return finish_(i2c, bus, timeout); }

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
    // Write Array - write count number of bytes from data array to Tx buffer
    // return: #bytes written = success, 0=fail
    // parameters:
    //      data = pointer to uint8_t (or char) array of data
    //      count = number of bytes to write
    //
    // As a special case C-strings (null terminated) can be written without a count byte
    //
    size_t write(const uint8_t* data, size_t count);
    inline size_t write(const char* str, size_t count) { return write((const uint8_t*)str, count); }
    inline size_t write(const char* str) { return write((const uint8_t*)str, strlen(str)+1); } // write C-string

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
    // Read Array - read count number of bytes from Rx buffer to data array
    // return: #bytes read
    // parameters:
    //      data = pointer to uint8_t (or char) array of data
    //      count = number of bytes to read
    //
    static size_t read_(struct i2cStruct* i2c, uint8_t* data, size_t count);
    inline size_t read(uint8_t* data, size_t count) { return read_(i2c, data, count); }
    inline size_t read(char* str, size_t count) { return read_(i2c, (uint8_t*)str, count); }

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
    // Set callback function for Master Tx
    //
    inline void onTransmitDone(void (*function)(void)) { i2c->user_onTransmitDone = function; }

    // ------------------------------------------------------------------------------------------------------
    // Set callback function for Master Rx
    //
    inline void onReqFromDone(void (*function)(void)) { i2c->user_onReqFromDone = function; }

    // ------------------------------------------------------------------------------------------------------
    // Set callback function for Slave Rx
    //
    inline void onReceive(void (*function)(size_t len)) { i2c->user_onReceive = function; }

    // ------------------------------------------------------------------------------------------------------
    // Set callback function for Slave Tx
    //
    inline void onRequest(void (*function)(void)) { i2c->user_onRequest = function; }

    // ------------------------------------------------------------------------------------------------------
    // Set callback function for Errors
    //
    inline void onError(void (*function)(void)) { i2c->user_onError = function; }

    // ------------------------------------------------------------------------------------------------------
    // Get error count from specified counter
    // return: error count
    // parameters:
    //      counter = I2C_ERRCNT_RESET_BUS, I2C_ERRCNT_TIMEOUT, I2C_ERRCNT_ADDR_NAK, I2C_ERRCNT_DATA_NAK,
    //                I2C_ERRCNT_ARBL, I2C_ERRCNT_NOT_ACQ, I2C_ERRCNT_DMA_ERR
    //
    inline uint32_t getErrorCount(i2c_err_count counter) { return i2c->errCounts[counter]; }
    // ------------------------------------------------------------------------------------------------------
    // Zero error count of specified counter
    // return: none
    // parameters:
    //      counter = I2C_ERRCNT_RESET_BUS, I2C_ERRCNT_TIMEOUT, I2C_ERRCNT_ADDR_NAK, I2C_ERRCNT_DATA_NAK,
    //                I2C_ERRCNT_ARBL, I2C_ERRCNT_NOT_ACQ, I2C_ERRCNT_DMA_ERR
    //
    inline void zeroErrorCount(i2c_err_count counter) { i2c->errCounts[counter] = 0; }

    // ------------------------------------------------------------------------------------------------------
    // For compatibility with pre-1.0 sketches and libraries
    inline void send(uint8_t b)             { write(b); }
    inline void send(uint8_t* s, uint8_t n) { write(s, n); }
    inline void send(int n)                 { write((uint8_t)n); }
    inline void send(char* s)               { write(s); }
    inline uint8_t receive(void)            { int c = read(); return (c<0) ? 0 : c; }
};

extern i2c_t3 Wire;
#if I2C_BUS_NUM >= 2
    extern i2c_t3 Wire1;
#endif
#if I2C_BUS_NUM >= 3
    extern i2c_t3 Wire2;
#endif
#if I2C_BUS_NUM >= 4
    extern i2c_t3 Wire3;
#endif

#endif // I2C_T3_H

/*
   ------------------------------------------------------------------------------------------------------
   Changelog
   ------------------------------------------------------------------------------------------------------

    - (v11.0) Modified 01Dec18 by Brian (nox771 at gmail.com)
        - Added state variables and modified pinConfigure_() to recognize unconfigured SCL/SDA pins, 
          allowing for setSCL()/setSDA() prior to begin(), which was previously blocked by bus busy 
          check on unconfigured pins.
        - Header change to MIT permissive license

    - (v10.1) Modified 02Jan18 by Brian (nox771 at gmail.com)
        - Added User #define to disable priority checks entirely
        - Added i2c_t3::isrActive flag to dynamically disable priority checks during ISR & callbacks.
          This is to prevent runaway priority escalation in cases of callbacks with nested Wire calls.

    - (v10.0) Modified 21Oct17 by Brian (nox771 at gmail.com)
        - Default assignments have been added to many functions for pins/pullup/rate/op_mode, so
          all those parameters are now optional in many function calls (marked ^ below)
        - Unbound SCL/SDA pin assignment.  Pins can be specified with either i2c_pins enum or by direct
          SCL,SDA pin definition (using any valid SCL and SDA pin).  New function summary is:
            - begin(mode, address1, ^i2c_pins, ^i2c_pullup, ^rate, ^i2c_op_mode)
            - begin(mode, address1, ^pinSCL, ^pinSDA, ^i2c_pullup, ^rate, ^i2c_op_mode)
            - pinConfigure(i2c_pins, ^pullup)
            - pinConfigure(pinSCL, pinSDA, ^pullup)
            - setSCL(pin)
            - setSDA(pin)
            - getSCL()
            - getSDA()
          Note: internal to i2c structure, currentPins has been replaced by currentSCL and currentSDA
        - Added Master callback functions for completion of transfers.  Primarily for
          sendTransmission/sendRequest, but these will also work on foreground commands
          endTransmission/requestFrom.  Also added an Error callback for Master bus errors.
            - onTransmitDone(function) - where function() is called when Master Transmit is complete
            - onReqFromDone(function) - where function() is called when Master Receive is complete
            - onError(function) - where function() is called upon any I2C error which terminates the
                                  Master bus operation (eg. NAK, timeout, acquire fail, etc)
        - Fixed blocking conditions that could occur in immediate mode
        - Added error counters which may be optionally enabled via I2C_ERROR_COUNTERS define.  When
          enabled it will track (Master mode only): Reset Bus (auto-retry only), Timeout, Addr NAK,
          Data NAK, Arb Lost, Bus Not Acquired, DMA Errors.
            - i2c_err_count enum, getErrorCount(), and zeroErrorCount() functions added

    - (v9.4) Modified 01Oct17 by Brian (nox771 at gmail.com)
        - Fixed Slave ISR for LC/3.5/3.6 not properly recognizing RepSTART
        - Fixed nested Wire calls during Slave ISR receive (calling Wire inside Wire1 Slave ISR)
        - Added uint8_t and char array read functions - Wire.read(databuf, count);

    - (v9.3) Modified 20Sep17 by Brian (nox771 at gmail.com)
        - Fixed Slave ISR for LC/3.5/3.6

    - (v9.2) Modified 29Dec16 by Brian (nox771 at gmail.com)
        - improved resetBus() function to reset C1 state (thanks hw999)

    - (v9.1) Modified 16Oct16 by Brian (nox771 at gmail.com)
        - applied two fixes due to bug reports:
            - removed I2C_F_DIV120 setting (120 divide-ratio) for I2C clock
            - disabled I2C_AUTO_RETRY by default (setting remains but must be manually enabled)

    - (v9) Modified 01Jul16 by Brian (nox771 at gmail.com)
        - Added support for Teensy 3.5/3.6:
            - fully supported (Master/Slave modes, IMM/ISR/DMA operation)
            - supports all available pin/bus options on Wire/Wire1/Wire2/Wire3
        - Fixed LC slave bug, whereby it was incorrectly detecting STOPs directed to other slaves
        - I2C rate is now set using a much more flexible method than previously used (this is partially
          motivated by increasing device count and frequencies).  As a result, the fixed set of rate
          enums are no longer needed (however they are currently still supported), and desired I2C
          frequency can be directly specified, eg. for 400kHz, I2C_RATE_400 can be replaced by 400000.
          Some setRate() functions are deprecated due to these changes.

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
*/
