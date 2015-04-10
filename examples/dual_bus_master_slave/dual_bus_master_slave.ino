// -------------------------------------------------------------------------------------------
// Teensy3.1/LC I2C Dual Bus Master Slave
// 18Jan14 Brian (nox771 at gmail.com)
// -------------------------------------------------------------------------------------------
//
// **********************************************************************************************
// ** Note: This is a Teensy 3.1/LC ONLY sketch since it requires a device with two I2C buses. **
// **********************************************************************************************
//
// This creates a device using one I2C bus as a Master and one I2C bus as a Slave.
//
// The "Wire" bus will be setup as a Master on pins 18(SDA0)/19(SCL0).
// The "Wire1" bus will be setup as a Slave on pins 29(SCL1)/30(SDA1) (T3.1) or 22(SCL1)/23(SDA1) (LC)
//
// The device can be setup to talk to other devices, or it can connect the buses together
// and talk to itself.  This allows Master/Slave development on a single device.  The example
// is setup for external pullups, so it will require external resistors on SCL & SDA.
//
// Pulling pin12 low will initiate a WRITE then READ transfer between Master and Slave.
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
void receiveEvent(size_t len);
void requestEvent(void);
void printWireStatus(void);
void printWire1Status(void);
void printStatus(i2c_status status);

// Slave Memory
#define MEM_LEN 256
uint8_t mem[MEM_LEN];
uint8_t cmd;
size_t addr;
i2c_rate rate;

void setup()
{
    pinMode(LED_BUILTIN,OUTPUT);    // LED
    pinMode(12,INPUT_PULLUP);       // Control for Test1

    // Wire - Setup for Master mode, pins 18/19, external pullup, 400kHz
    Wire.begin(I2C_MASTER, 0x00, I2C_PINS_18_19, I2C_PULLUP_EXT, I2C_RATE_400);

    // Wire1 - Setup for Slave mode, address 0x44, pins 29/30 (T3.1) or 22/23 (LC),
    //         external pullup, 400kHz
    #if defined(__MK20DX256__)
        Wire1.begin(I2C_SLAVE, 0x44, I2C_PINS_29_30, I2C_PULLUP_EXT, I2C_RATE_400);
    #else
        Wire1.begin(I2C_SLAVE, 0x44, I2C_PINS_22_23, I2C_PULLUP_EXT, I2C_RATE_400);
    #endif
    // Slave init
    cmd = 0;
    addr = 0;
    memset(mem, 0, sizeof(mem));

    // register Slave events
    Wire1.onReceive(receiveEvent);
    Wire1.onRequest(requestEvent);

    Serial.begin(115200);
}

//
// Master Loop
//
void loop()
{
    uint8_t target = 0x44;  // target address
    uint8_t databuf[256];
    uint8_t addr = 0, len;

    //
    // Test1 - pull pin12 low to send command to slave
    //

    if(digitalRead(12) == LOW)
    {
        uint8_t fail, data;

        digitalWrite(LED_BUILTIN,HIGH);         // LED on
        Serial.print("------------------------------------------------------------\n");
        Serial.print("Test1 : This will WRITE then READ a 32 byte block from\n");
        Serial.print("        the Master device (Wire) to the Slave device (Wire1)\n");
        Serial.print("------------------------------------------------------------\n");

        for(len = 0; len < 32; len++)           // prepare data to send
            databuf[len] = random(0xFF);        // set random data

        Serial.printf("I2C WRITE 32 bytes to Slave 0x%0X at MemAddr %d\n", target, addr);
        Serial.print("Writing: ");
        for(len = 0; len < 32; len++) { Serial.printf("%d ",databuf[len]); }
        Serial.print("\n");

        Wire.beginTransmission(target);         // slave addr
        Wire.write(WRITE);                      // WRITE command
        Wire.write(addr);                       // memory address
        for(len = 0; len < 32; len++)           // write 32 byte block
            Wire.write(databuf[len]);
        Wire.endTransmission();                 // blocking write (when not specified I2C_STOP is implicit)
        fail = Wire.getError();

        printWireStatus();                      // print I2C final status

        // Reading from Slave ------------------------------------------------------
        if(!fail)
        {
            Wire.beginTransmission(target);         // slave addr
            Wire.write(READ);                       // READ command
            Wire.write(addr);                       // memory address
            Wire.endTransmission(I2C_NOSTOP);       // blocking write (NOSTOP triggers RepSTART on next I2C command)
            Wire.requestFrom(target,32,I2C_STOP);   // blocking read (request 32 bytes)
            fail = Wire.getError();

            Serial.printf("I2C READ %d bytes from Slave 0x%0X at MemAddr %d\n", Wire.available(), target, addr);
            Serial.print("Received: ");             // print received bytes
            for(len = 0; len < 32 && Wire.available(); len++)   // verify 32 byte block
            {
                data = Wire.readByte();
                Serial.printf("%d ", data);
                if(databuf[len] != data) fail=1;
            }
            if(!fail)
                Serial.print("\nCORRECT!\n");
            else
                Serial.print("\nWRONG!\n");
        }
        printWireStatus();                      // print I2C final status

        delay(500);                             // delay to space out tests
        digitalWrite(LED_BUILTIN,LOW);          // LED off
    }
}

//
// handle Slave Rx Event (incoming I2C request/data)
//
void receiveEvent(size_t len)
{
    if(Wire1.available())
    {
        // grab command
        cmd = Wire1.readByte();
        switch(cmd)
        {
        case WRITE:
            addr = Wire1.readByte();                // grab addr
            while(Wire1.available())
                if(addr < MEM_LEN)                  // drop data beyond mem boundary
                    mem[addr++] = Wire1.readByte(); // copy data to mem
                else
                    Wire1.readByte();               // drop data if mem full
            break;

        case READ:
            addr = Wire1.readByte();                // grab addr
            break;

        case SETRATE:
            rate = (i2c_rate)Wire1.readByte();      // grab rate
            Wire1.setRate(rate);                    // set rate
            break;
        }
    }
}

//
// handle Slave Tx Event (outgoing I2C data)
//
void requestEvent(void)
{
    switch(cmd)
    {
    case READ:
        Wire1.write(&mem[addr], MEM_LEN-addr); // fill Tx buffer (from addr location to end of mem)
        break;
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

