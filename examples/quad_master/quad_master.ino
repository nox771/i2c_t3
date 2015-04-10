// -------------------------------------------------------------------------------------------
// Teensy3.1 I2C Quad-Master
// 18Jan14 Brian (nox771 at gmail.com)
// -------------------------------------------------------------------------------------------
//
// *******************************************************************************************
// ** Note: This is a Teensy 3.1 ONLY sketch since it requires a device with two I2C buses, **
// **       each having two sets of multiplexed pins.                                       **
// *******************************************************************************************
//
// It creates an I2C quad-master device which utilizes both I2C buses, and both sets of pins
// for each bus, thereby creating a quad-master device.
//
// The "Wire" bus will communicate on pins 18(SDA0)/19(SCL0) and 16(SCL0)/17(SDA0).
// The "Wire1" bus will communicate on pins 29(SCL1)/30(SDA1) and 26(SCL1)/31(SDA1).
//
// This code assumes that each bus has a target device configured with the simple I2C slave
// given in the i2c_t3/slave sketch, with each having a 256byte memory and I2C addr 0x44.
//
// The various tests are started by pulling one of the control pins low as follows:
//
// pull pin9 low to send command to slave on bus1 (pins 18/19)
// pull pin10 low to send command to slave on bus2 (pins 16/17)
// pull pin11 low to send command to slave on bus3 (pins 29/30)
// pull pin12 low to send command to slave on bus4 (pins 26/31)
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
void printWireStatus(void);
void printWire1Status(void);
void printStatus(i2c_status status);

void setup()
{
    pinMode(LED_BUILTIN,OUTPUT);    // LED
    pinMode(9,INPUT_PULLUP);        // Control for Bus1
    pinMode(10,INPUT_PULLUP);       // Control for Bus2
    pinMode(11,INPUT_PULLUP);       // Control for Bus3
    pinMode(12,INPUT_PULLUP);       // Control for Bus4

    // Setup for Master mode, default pins, 400kHz, and internal pullups
    // (note: internal pullups only used here to reduce test board clutter with 4 buses, external generally better)
    //
    // Wire
    Wire.begin(I2C_MASTER, 0x00, I2C_PINS_18_19, I2C_PULLUP_INT, I2C_RATE_400);
    pinMode(16,INPUT_PULLUP); // Alternate Wire pins 16/17 should be initialized as inputs with same pullup style
    pinMode(17,INPUT_PULLUP);
    // Wire1
    Wire1.begin(I2C_MASTER, 0x00, I2C_PINS_29_30, I2C_PULLUP_INT, I2C_RATE_400);
    pinMode(26,INPUT_PULLUP); // Alternate Wire1 pins 26/31 should be initialized as inputs with same pullup style
    pinMode(31,INPUT_PULLUP);

    Serial.begin(115200);
}

void loop()
{
    // slave addr - note: using identical slave addresses is intentional, as this
    //                    is a common situation with many off-the-shelf devices
    uint8_t target = 0x44;
    uint8_t data = 0x0A, addr = 0;

    //
    // Quad-bus test
    //
    // pull pin9 low to send command to slave on bus1 (pins 18/19)
    // pull pin10 low to send command to slave on bus2 (pins 16/17)
    // pull pin11 low to send command to slave on bus3 (pins 29/30)
    // pull pin12 low to send command to slave on bus4 (pins 26/31)
    //

    if(digitalRead(12) == LOW)
    {
        // do a 1 byte write on bus1
        //
        Wire.pinConfigure(I2C_PINS_18_19, I2C_PULLUP_INT); // change pins

        digitalWrite(LED_BUILTIN,HIGH);         // LED on
        Serial.print("---------------------------------------------------------\n");
        Serial.print("Test1 : This will WRITE then READ 1 byte to the Slave\n");
        Serial.print("        connected to pins 18/19.\n");
        Serial.print("---------------------------------------------------------\n");
        Serial.printf("Writing 1 byte (0x%0X) to Slave 0x%0X at MemAddr %d\n", data, target, addr);

        Wire.beginTransmission(target);         // slave addr
        Wire.write(WRITE);                      // WRITE command
        Wire.write(addr);                       // memory address
        Wire.write(data);                       // set data
        Wire.endTransmission();                 // blocking I2C Tx (when not specified I2C_STOP is implicit)

        printWireStatus();                      // print I2C final status

        // Reading from Slave ------------------------------------------------------
        Wire.beginTransmission(target);         // slave addr
        Wire.write(READ);                       // READ command
        Wire.write(addr);                       // memory address
        Wire.endTransmission(I2C_NOSTOP);       // blocking write (NOSTOP triggers RepSTART on next I2C command)
        Wire.requestFrom(target,1,I2C_STOP);    // blocking read (request 1 byte)

        Serial.printf("Read 1 byte (0x%0X) from Slave 0x%0X at MemAddr %d\n", Wire.readByte(), target, addr);
        printWireStatus();                      // print I2C final status
        digitalWrite(LED_BUILTIN,LOW);          // LED off
        delay(500); // delay to space out tests
    }

    if(digitalRead(11) == LOW)
    {
        // do a 1 byte write on bus2
        //
        Wire.pinConfigure(I2C_PINS_16_17, I2C_PULLUP_INT); // change pins

        digitalWrite(LED_BUILTIN,HIGH);         // LED on
        Serial.print("---------------------------------------------------------\n");
        Serial.print("Test2 : This will WRITE then READ 1 byte to the Slave\n");
        Serial.print("        connected to pins 16/17.\n");
        Serial.print("---------------------------------------------------------\n");
        Serial.printf("Writing 1 byte (0x%0X) to Slave 0x%0X at MemAddr %d\n", data, target, addr);

        Wire.beginTransmission(target);         // slave addr
        Wire.write(WRITE);                      // WRITE command
        Wire.write(addr);                       // memory address
        Wire.write(data);                       // set data
        Wire.endTransmission();                 // blocking I2C Tx (when not specified I2C_STOP is implicit)

        printWireStatus();                      // print I2C final status

        // Reading from Slave ------------------------------------------------------
        Wire.beginTransmission(target);         // slave addr
        Wire.write(READ);                       // READ command
        Wire.write(addr);                       // memory address
        Wire.endTransmission(I2C_NOSTOP);       // blocking write (NOSTOP triggers RepSTART on next I2C command)
        Wire.requestFrom(target,1,I2C_STOP);    // blocking read (request 1 byte)

        Serial.printf("Read 1 byte (0x%0X) from Slave 0x%0X at MemAddr %d\n", Wire.readByte(), target, addr);
        printWireStatus();                      // print I2C final status
        digitalWrite(LED_BUILTIN,LOW);          // LED off
        delay(500); // delay to space out tests
    }

    if(digitalRead(10) == LOW)
    {
        // do a 1 byte write on bus3
        //
        Wire1.pinConfigure(I2C_PINS_29_30, I2C_PULLUP_INT); // change pins

        digitalWrite(LED_BUILTIN,HIGH);         // LED on
        Serial.print("---------------------------------------------------------\n");
        Serial.print("Test3 : This will WRITE then READ 1 byte to the Slave\n");
        Serial.print("        connected to pins 29/30.\n");
        Serial.print("---------------------------------------------------------\n");
        Serial.printf("Writing 1 byte (0x%0X) to Slave 0x%0X at MemAddr %d\n", data, target, addr);

        Wire1.beginTransmission(target);        // slave addr
        Wire1.write(WRITE);                     // WRITE command
        Wire1.write(addr);                      // memory address
        Wire1.write(data);                      // set data
        Wire1.endTransmission();                // blocking I2C Tx (when not specified I2C_STOP is implicit)

        printWire1Status();                     // print I2C final status

        // Reading from Slave ------------------------------------------------------
        Wire1.beginTransmission(target);        // slave addr
        Wire1.write(READ);                      // READ command
        Wire1.write(addr);                      // memory address
        Wire1.endTransmission(I2C_NOSTOP);      // blocking write (NOSTOP triggers RepSTART on next I2C command)
        Wire1.requestFrom(target,1,I2C_STOP);   // blocking read (request 1 byte)

        Serial.printf("Read 1 byte (0x%0X) from Slave 0x%0X at MemAddr %d\n", Wire1.readByte(), target, addr);
        printWire1Status();                     // print I2C final status
        digitalWrite(LED_BUILTIN,LOW);          // LED off
        delay(500); // delay to space out tests
    }

    if(digitalRead(9) == LOW)
    {
        // do a 1 byte write on bus4
        //
        Wire1.pinConfigure(I2C_PINS_26_31, I2C_PULLUP_INT); // change pins

        digitalWrite(LED_BUILTIN,HIGH);         // LED on
        Serial.print("---------------------------------------------------------\n");
        Serial.print("Test4 : This will WRITE then READ 1 byte to the Slave\n");
        Serial.print("        connected to pins 26/31.\n");
        Serial.print("---------------------------------------------------------\n");
        Serial.printf("Writing 1 byte (0x%0X) to Slave 0x%0X at MemAddr %d\n", data, target, addr);

        Wire1.beginTransmission(target);        // slave addr
        Wire1.write(WRITE);                     // WRITE command
        Wire1.write(addr);                      // memory address
        Wire1.write(data);                      // set data
        Wire1.endTransmission();                // blocking I2C Tx (when not specified I2C_STOP is implicit)

        printWire1Status();                     // print I2C final status

        // Reading from Slave ------------------------------------------------------
        Wire1.beginTransmission(target);        // slave addr
        Wire1.write(READ);                      // READ command
        Wire1.write(addr);                      // memory address
        Wire1.endTransmission(I2C_NOSTOP);      // blocking write (NOSTOP triggers RepSTART on next I2C command)
        Wire1.requestFrom(target,1,I2C_STOP);   // blocking read (request 1 byte)

        Serial.printf("Read 1 byte (0x%0X) from Slave 0x%0X at MemAddr %d\n", Wire1.readByte(), target, addr);
        printWire1Status();                     // print I2C final status
        digitalWrite(LED_BUILTIN,LOW);          // LED off
        delay(500); // delay to space out tests
    }
}

//
// print I2C status
//
void printWireStatus(void) { printStatus(Wire.status()); }
void printWire1Status(void) { printStatus(Wire1.status()); }
void printStatus(i2c_status status)
{
    switch(status)
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

