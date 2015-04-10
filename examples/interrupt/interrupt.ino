// -------------------------------------------------------------------------------------------
// Teensy3.0/3.1/LC I2C Interrupt Test
// 15May13 Brian (nox771 at gmail.com)
// -------------------------------------------------------------------------------------------
//
// This creates an I2C master device which will issue I2C commands from inside a periodic
// interrupt (eg. reading a sensor at regular time intervals).  For this test the Slave device
// will be assumed to be that given in the i2c_t3/slave sketch.
//
// The test will start when the Serial monitor opens.
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
void readSlave(void);

IntervalTimer slaveTimer;
uint8_t target=0x44;
size_t addr=0, len;

void setup()
{
    pinMode(LED_BUILTIN,OUTPUT);        // LED

    // Setup for Master mode, pins 18/19, external pullups, 400kHz
    Wire.begin(I2C_MASTER, 0x00, I2C_PINS_18_19, I2C_PULLUP_EXT, I2C_RATE_400);

    Serial.begin(115200);
    delay(5);                           // Slave powerup wait

    // Setup Slave
    Wire.beginTransmission(target);     // slave addr
    Wire.write(WRITE);                  // WRITE command
    Wire.write(addr);                   // memory address
    for(len = 0; len < 8; len++)        // write 8 byte block
        Wire.write(len);
    Wire.endTransmission();             // blocking Tx

    while(!Serial);                     // wait to start

    // Start reading Slave device
    slaveTimer.begin(readSlave,1000000); // 1s timer
}

void loop()
{
    delay(1);
}

void readSlave(void)
{
    digitalWrite(LED_BUILTIN,HIGH);         // pulse LED when reading

    Wire.beginTransmission(target);         // slave addr
    Wire.write(READ);                       // WRITE command
    Wire.write(addr);                       // memory address
    Wire.endTransmission(I2C_NOSTOP);       // blocking Tx, no STOP
    Wire.requestFrom(target,1,I2C_STOP);    // READ 1 byte

    Serial.printf("Data read from Slave device: %d\n", Wire.readByte());
    addr = (addr < 7) ? addr+1 : 0;         // loop reading memory address
    digitalWrite(LED_BUILTIN,LOW);
}
