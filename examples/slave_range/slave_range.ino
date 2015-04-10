// -------------------------------------------------------------------------------------------
// Teensy3.0/3.1/LC I2C Slave w/Address Range
// 08Mar13 Brian (nox771 at gmail.com)
// -------------------------------------------------------------------------------------------
//
// This creates an I2C slave device which will respond to a range of I2C addresses.  It will
// dump any data it receives to Serial.
//
// The getRxAddr() function can be used to obtain the Slave address in the callback.  This
// could then be used to alter Slave response and make a single device then appear as multiple
// different Slave targets.
//
// This example code is in the public domain.
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
void receiveEvent(size_t len);
void requestEvent(void);

// Memory
#define MEM_LEN 256
uint8_t mem[MEM_LEN];
uint8_t cmd, data, target;
size_t addr;
i2c_rate rate;

//
// Setup
//
void setup()
{
    pinMode(LED_BUILTIN,OUTPUT); // LED

    Serial.begin(115200);

    // Setup for Slave mode, addresses 0x08 to 0x77, pins 18/19, external pullups, 400kHz
    Wire.begin(I2C_SLAVE, 0x08, 0x77, I2C_PINS_18_19, I2C_PULLUP_EXT, I2C_RATE_400);

    // init vars
    cmd = 0;
    addr = 0;
    memset(mem, 0, sizeof(mem));
    rate = I2C_RATE_400;

    // register events
    Wire.onReceive(receiveEvent);
    Wire.onRequest(requestEvent);
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
void receiveEvent(size_t len)
{
    if(Wire.available())
    {
        // grab command
        target = Wire.getRxAddr();                  // getRxAddr() is used to obtain Slave address
        cmd = Wire.readByte();
        switch(cmd)
        {
        case WRITE:
            addr = Wire.readByte();                 // grab mem addr

            Serial.printf("WRITE to Slave 0x%0X at MemAddr: %d  Data: ",target,addr);
            while(Wire.available())
                if(addr < MEM_LEN)                  // drop data beyond mem boundary
                {
                    data = Wire.readByte();         // grab data
                    mem[addr++] = data;             // copy data to mem
                    Serial.printf("%d ",data);
                }
                else
                    Wire.readByte();                // drop data if mem full
            Serial.print("\n");
            break;

        case READ:
            target = Wire.getRxAddr();
            addr = Wire.readByte();                 // grab addr
            Serial.printf("READ from Slave 0x%0X at MemAddr: %d\n",target,addr);
            break;

        case SETRATE:
            rate = (i2c_rate)Wire.readByte();       // grab rate
            Wire.setRate(rate);                     // set rate
            Serial.print("Rate changed\n");
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
