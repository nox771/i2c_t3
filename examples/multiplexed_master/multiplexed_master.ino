// -------------------------------------------------------------------------------------------
// Teensy3.0/3.1/LC I2C Multiplexed Master
// 08Mar13 Brian (nox771 at gmail.com)
// -------------------------------------------------------------------------------------------
//
// This creates an I2C multiplexed master device which talks to the simple I2C slave device
// given in the i2c_t3/slave sketch.  It is different than the Master sketch because it can
// output I2C commands on either pins 18/19 or pins 16/17, and change pins on-the-fly.
//
// Note that it is using the same I2C bus on both sets of pins (I2C0).  The purpose of this
// sketch is to demonstrate how the pins can be reconfigured on-the-fly when the bus is idle.
//
// This code assumes the slave config has 256byte memory and I2C address is 0x44.
// The various tests are started by pulling one of the control pins low.
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
    pinMode(12,INPUT_PULLUP);       // Control for Test1
    pinMode(11,INPUT_PULLUP);       // Control for Test2

    // Setup for Master mode, pins 18/19, external pullups, 400kHz
    Wire.begin(I2C_MASTER, 0x00, I2C_PINS_18_19, I2C_PULLUP_EXT, I2C_RATE_400);

    Serial.begin(115200);
}

void loop()
{
    uint8_t data = 0x0A, addr = 0;
    uint8_t target1 = 0x44; // slave1 addr
    uint8_t target2 = 0x44; // slave2 addr

    //
    // Multi-bus test
    //
    // pull pin12 low to send command to slave1 on bus1 (pins 16/17)
    // pull pin11 low to send command to slave2 on bus2 (pins 18/19)
    //

    if(digitalRead(12) == LOW)
    {
        // do a 1 byte write on bus1
        //
        Wire.pinConfigure(I2C_PINS_16_17, I2C_PULLUP_EXT); // change pins

        digitalWrite(LED_BUILTIN,HIGH);         // LED on
        Serial.print("---------------------------------------------------------\n");
        Serial.print("Test1 : This will WRITE then READ 1 byte to the Slave\n");
        Serial.print("        connected to pins 16/17.\n");
        Serial.print("---------------------------------------------------------\n");
        Serial.printf("Writing 1 byte (0x%0X) to Slave 0x%0X at MemAddr %d\n", data, target1, addr);

        Wire.beginTransmission(target1);        // slave addr
        Wire.write(WRITE);                      // WRITE command
        Wire.write(addr);                       // memory address
        Wire.write(data);                       // set data
        Wire.endTransmission();                 // blocking I2C Tx (when not specified I2C_STOP is implicit)

        print_i2c_status();                     // print I2C final status

        // Reading from Slave ------------------------------------------------------
        Wire.beginTransmission(target1);        // slave addr
        Wire.write(READ);                       // READ command
        Wire.write(addr);                       // memory address
        Wire.endTransmission(I2C_NOSTOP);       // blocking write (NOSTOP triggers RepSTART on next I2C command)
        Wire.requestFrom(target1,1,I2C_STOP);   // blocking read (request 1 byte)

        Serial.printf("Read 1 byte (0x%0X) from Slave 0x%0X at MemAddr %d\n", Wire.readByte(), target1, addr);
        print_i2c_status();                     // print I2C final status
        digitalWrite(LED_BUILTIN,LOW);          // LED off
        delay(500); // delay to space out tests
    }

    if(digitalRead(11) == LOW)
    {
        // do a 1 byte write on bus2
        //
        Wire.pinConfigure(I2C_PINS_18_19, I2C_PULLUP_EXT); // change pins

        digitalWrite(LED_BUILTIN,HIGH);         // LED on
        Serial.print("---------------------------------------------------------\n");
        Serial.print("Test2 : This will WRITE then READ 1 byte to the Slave\n");
        Serial.print("        connected to pins 18/19.\n");
        Serial.print("---------------------------------------------------------\n");
        Serial.printf("Writing 1 byte (0x%0X) to Slave 0x%0X at MemAddr %d\n", data, target2, addr);

        Wire.beginTransmission(target2);        // slave addr
        Wire.write(WRITE);                      // WRITE command
        Wire.write(addr);                       // memory address
        Wire.write(data);                       // set data
        Wire.endTransmission();                 // blocking I2C Tx (when not specified I2C_STOP is implicit)

        print_i2c_status();                     // print I2C final status

        // Reading from Slave ------------------------------------------------------
        Wire.beginTransmission(target2);        // slave addr
        Wire.write(READ);                       // READ command
        Wire.write(addr);                       // memory address
        Wire.endTransmission(I2C_NOSTOP);       // blocking write (NOSTOP triggers RepSTART on next I2C command)
        Wire.requestFrom(target2,1,I2C_STOP);   // blocking read (request 1 byte)

        Serial.printf("Read 1 byte (0x%0X) from Slave 0x%0X at MemAddr %d\n", Wire.readByte(), target2, addr);
        print_i2c_status();                     // print I2C final status
        digitalWrite(LED_BUILTIN,LOW);          // LED off
        delay(500); // delay to space out tests
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
