// -------------------------------------------------------------------------------------------
// I2C Advanced Slave
// -------------------------------------------------------------------------------------------
//
// This creates an I2C slave device with simple read/write commands and a small
// addressable memory.  Note that this communication adds a protocol layer on top of
// normal I2C read/write procedures.  As such, it is meant to pair with the advanced_master 
// sketch.  The read/write commands are described below.
//
// For basic I2C communication only, refer to basic_master and basic_slave example sketches.
//
// This example code is in the public domain.
//
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
// START|I2CADDR+W|SETRATE|RATE0|RATE1|RATE2|RATE3|STOP
//
// where START     = I2C START sequence
//       I2CADDR+W = I2C Slave address + I2C write flag
//       SETRATE   = SETRATE command
//       RATE0-3   = I2C frequency (uint32_t) LSB-to-MSB format
// -------------------------------------------------------------------------------------------

#include <i2c_t3.h>

// Command definitions
#define WRITE    0x10
#define READ     0x20
#define SETRATE  0x30

// Function prototypes
void receiveEvent(size_t count);
void requestEvent(void);

// Memory
#define MEM_LEN 256
uint8_t mem[MEM_LEN];
volatile uint8_t cmd;
volatile size_t addr;
volatile uint32_t rate;

//
// Setup
//
void setup()
{
    pinMode(LED_BUILTIN,OUTPUT); // LED

    // Setup for Slave mode, address 0x44, pins 18/19, external pullups, 400kHz
    Wire.begin(I2C_SLAVE, 0x44, I2C_PINS_18_19, I2C_PULLUP_EXT, 400000);

    // init vars
    cmd = 0;
    addr = 0;
    memset(mem, 0, sizeof(mem));
    rate = 400000;

    // register events
    Wire.onReceive(receiveEvent);
    Wire.onRequest(requestEvent);

    Serial.begin(115200);
}

void loop()
{
    digitalWrite(LED_BUILTIN,HIGH); // double pulse LED while waiting for I2C requests
    delay(10);                      // if the LED stops the slave is probably stuck in an ISR
    digitalWrite(LED_BUILTIN,LOW);
    delay(100);
    digitalWrite(LED_BUILTIN,HIGH);
    delay(10);
    digitalWrite(LED_BUILTIN,LOW);
    delay(880);
}

//
// handle Rx Event (incoming I2C request/data)
//
void receiveEvent(size_t count)
{
    if(count)
    {
        // grab command
        cmd = Wire.readByte();
        switch(cmd)
        {
        case WRITE:
            addr = Wire.readByte();         // grab addr
            Wire.read(&mem[addr], count-2); // copy Rx data to databuf
            break;

        case READ:
            addr = Wire.readByte();                // grab addr
            break;

        case SETRATE:
            if(Wire.available() >= 4)
            {
                rate = Wire.readByte();            // grab rate
                rate |= Wire.readByte() << 8;
                rate |= Wire.readByte() << 16;
                rate |= Wire.readByte() << 24;
                Wire.setClock(rate); // set rate
            }
            break;
        }
    }
}

//
// handle Tx Event (outgoing I2C data)
//
void requestEvent(void)
{
    switch(cmd)
    {
    case READ:
        Wire.write(&mem[addr], MEM_LEN-addr); // fill Tx buffer (from addr location to end of mem)
        break;
    }
}

