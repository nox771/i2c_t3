// -------------------------------------------------------------------------------------------
// Teensy3.0/3.1/LC I2C Timeout Test
// 15May13 Brian (nox771 at gmail.com)
// -------------------------------------------------------------------------------------------
//
// This creates an I2C master device which talks to the simple I2C slave device given in the
// i2c_t3/slave sketch.  It tests the various timeout functions.
//
// This code assumes the slave config has >8byte memory and I2C addr is 0x44.
// The test is started by pulling the control pin low.
//
// This example code is in the public domain.
//
// -------------------------------------------------------------------------------------------
// Slave protocol is as follows:
// -------------------------------------------------------------------------------------------
// WRITE - The I2C Master can write to the device by transmitting the WRITE command,
//         a memory address to store to, and a sequence of data to store.
//         The command sequence is:
//
// START|I2CADDR+W|WRITE|MEMADDR|DATA0|DATA1|DATA2|...|STOP
//
// where START     = I2C START sequence
//       I2CADDR+W = I2C Slave address + I2C write flag
//       WRITE     = WRITE command
//       MEMADDR   = memory address to store data to
//       DATAx     = data byte to store, multiple bytes are stored to increasing address
//       STOP      = I2C STOP sequence
// -------------------------------------------------------------------------------------------
// READ - The I2C Master can read data from the device by transmitting the READ command,
//        a memory address to read from, and then issuing a STOP/START or Repeated-START,
//        followed by reading the data.  The command sequence is:
//
// START|I2CADDR+W|READ|MEMADDR|REPSTART|I2CADDR+R|DATA0|DATA1|DATA2|...|STOP
//
// where START     = I2C START sequence
//       I2CADDR+W = I2C Slave address + I2C write flag
//       READ      = READ command
//       MEMADDR   = memory address to read data from
//       REPSTART  = I2C Repeated-START sequence (or STOP/START if single Master)
//       I2CADDR+R = I2C Slave address + I2C read flag
//       DATAx     = data byte read by Master, multiple bytes are read from increasing address
//       STOP      = I2C STOP sequence
// -------------------------------------------------------------------------------------------
// SETRATE - The I2C Master can adjust the Slave configured I2C rate with this command
//           The command sequence is:
//
// START|I2CADDR+W|SETRATE|RATE|STOP
//
// where START     = I2C START sequence
//       I2CADDR+W = I2C Slave address + I2C write flag
//       SETRATE   = SETRATE command
//       RATE      = I2C RATE to use (must be from i2c_rate enum list, eg. I2C_RATE_xxxx)
// -------------------------------------------------------------------------------------------

#include <i2c_t3.h>

// Command definitions
#define WRITE    0x10
#define READ     0x20
#define SETRATE  0x30

// Function prototypes
void print_i2c_status(void);

void setup()
{
    pinMode(LED_BUILTIN,OUTPUT);    // LED
    pinMode(12,INPUT_PULLUP);       // Control for Test sequence

    // Setup for Master mode, pins 18/19, external pullups, 400kHz
    Wire.begin(I2C_MASTER, 0x00, I2C_PINS_18_19, I2C_PULLUP_EXT, I2C_RATE_400);

    Serial.begin(115200);
}

void loop()
{
    uint8_t target = 0x44;
    size_t addr, len;
    uint8_t databuf[256];
    elapsedMicros deltaT;
    uint32_t hold;

    if(digitalRead(12) == LOW)
    {
        digitalWrite(LED_BUILTIN,HIGH);             // LED on
        Serial.print("---------------------------------------------------------\n");
        Serial.print("Test1 : Blocking transmit with 50us timeout\n");
        Serial.print("---------------------------------------------------------\n");

        // Writing to Slave --------------------------------------------------------
        addr = 0;
        for(len = 0; len < 8; len++)
            databuf[len] = len;                     // prepare data, set data == address
        Serial.printf("I2C WRITE 8 bytes to Slave 0x%0X at MemAddr %d (50us timeout)\n", target, addr);

        Wire.beginTransmission(target);             // slave addr
        Wire.write(WRITE);                          // WRITE command
        Wire.write(addr);                           // memory address
        for(len = 0; len < 8; len++)                // write 8 byte block
            Wire.write(databuf[len]);
        deltaT = 0;
        Wire.endTransmission(I2C_STOP,50);          // blocking Tx with 50us timeout
        hold = deltaT;

        print_i2c_status();                         // print I2C final status
        Serial.printf("Duration: %dus\n", hold);    // print duration
        delay(1);                                   // delay to space out tests

        Serial.print("---------------------------------------------------------\n");
        Serial.print("Test2 : Blocking transmit with no timeout\n");
        Serial.print("---------------------------------------------------------\n");
        Serial.printf("I2C WRITE 8 bytes to Slave 0x%0X at MemAddr %d (no timeout)\n", target, addr);

        Wire.beginTransmission(target);             // slave addr
        Wire.write(WRITE);                          // WRITE command
        Wire.write(addr);                           // memory address
        for(len = 0; len < 8; len++)                // write 8 byte block
            Wire.write(databuf[len]);
        deltaT = 0;
        Wire.endTransmission(I2C_STOP);             // blocking Tx with no timeout
        hold = deltaT;

        print_i2c_status();                         // print I2C final status
        Serial.printf("Duration: %dus\n", hold);    // print duration
        delay(1);                                   // delay to space out tests

        Serial.print("---------------------------------------------------------\n");
        Serial.print("Test3 : Blocking receive with 50us timeout\n");
        Serial.print("---------------------------------------------------------\n");
        Serial.printf("I2C READ 8 bytes from Slave 0x%0X at MemAddr %d (50us timeout)\n", target, addr);
        Serial.print("Note: resulting timeout duration is longer since this takes 2 commands WRITE then READ\n");

        Wire.beginTransmission(target);             // slave addr
        Wire.write(READ);                           // WRITE command
        Wire.write(addr);                           // memory address
        deltaT = 0;
        Wire.endTransmission(I2C_NOSTOP);           // blocking Tx, no STOP
        Wire.requestFrom(target,8,I2C_STOP,50);     // blocking Rx with 50us timeout
        hold = deltaT;

        print_i2c_status();                         // print I2C final status
        Serial.printf("Duration: %dus\n", hold);    // print duration
        delay(1);                                   // delay to space out tests

        Serial.print("---------------------------------------------------------\n");
        Serial.print("Test4 : Blocking receive with no timeout\n");
        Serial.print("---------------------------------------------------------\n");
        Serial.printf("I2C READ 8 bytes from Slave 0x%0X at MemAddr %d (no timeout)\n", target, addr);

        Wire.beginTransmission(target);             // slave addr
        Wire.write(READ);                           // WRITE command
        Wire.write(addr);                           // memory address
        deltaT = 0;
        Wire.endTransmission(I2C_NOSTOP);           // blocking Tx, no STOP
        Wire.requestFrom(target,8,I2C_STOP);        // blocking Rx with no timeout
        hold = deltaT;

        print_i2c_status();                         // print I2C final status
        Serial.printf("Duration: %dus\n", hold);    // print duration
        delay(1);                                   // delay to space out tests

        Serial.print("---------------------------------------------------------\n");
        Serial.print("Test5 : Non-blocking transmit with 50us finish() timeout\n");
        Serial.print("---------------------------------------------------------\n");
        Serial.printf("I2C WRITE 8 bytes to Slave 0x%0X at MemAddr %d (50us finish() timeout)\n", target, addr);

        Wire.beginTransmission(target);             // slave addr
        Wire.write(WRITE);                          // WRITE command
        Wire.write(addr);                           // memory address
        for(len = 0; len < 8; len++)                // write 8 byte block
            Wire.write(databuf[len]);
        deltaT = 0;
        Wire.sendTransmission(I2C_STOP);            // non-blocking Tx
        //
        // I2C working in background, so do other stuff here
        //
        Wire.finish(50);                            // finish with timeout
        hold = deltaT;

        print_i2c_status();                         // print I2C final status
        Serial.printf("Duration: %dus\n", hold);    // print duration
        delay(1);                                   // delay to space out tests

        Serial.print("Done\n");
        digitalWrite(LED_BUILTIN,LOW);              // LED on
        delay(1000);                                // delay to space out tests
    }
}

//
// print I2C status
//
void print_i2c_status(void)
{
    switch(Wire.status())
    {
    case I2C_WAITING:  Serial.print("I2C waiting, no errors\n"); break;
    case I2C_ADDR_NAK: Serial.print("Slave addr not acknowledged\n"); break;
    case I2C_DATA_NAK: Serial.print("Slave data not acknowledged\n"); break;
    case I2C_ARB_LOST: Serial.print("Bus Error: Arbitration Lost\n"); break;
    case I2C_TIMEOUT:  Serial.print("I2C timeout\n"); break;
    case I2C_BUF_OVF:  Serial.print("I2C buffer overflow\n"); break;
    default:           Serial.print("I2C busy\n"); break;
    }
}
