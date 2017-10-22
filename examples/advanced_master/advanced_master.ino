// -------------------------------------------------------------------------------------------
// I2C Advanced Master
// -------------------------------------------------------------------------------------------
//
// This creates an I2C master device with simple read/write commands and a small
// addressable memory.  Note that this communication adds a protocol layer on top of
// normal I2C read/write procedures.  As such, it is meant to pair with the advanced_slave 
// sketch.  The read/write commands are described below.
//
// This code assumes the slave config has 256 byte memory and I2C addr is 0x44.
//
// Tests are as follows:
// Pull pin12 input low to send/receive 256 bytes to/from slave in 32 byte blocks.
// Pull pin11 input low to send/receive 256 bytes to/from slave in single block.
// Pull pin10 input low to send/receive 256 bytes to/from slave in single block, using 
//            non-blocking commands.
// Pull pin9 input low to send/receive 256 bytes to/from slave in single block, using 
//            non-blocking commands in DMA mode.
// Pull pin8 input low to run I2C rate sweep test.  This sweeps the I2C rates on both master 
//            and slave and times the duration of a 256 byte transfer at each rate.
//
// For basic I2C communication only, refer to basic_master and basic_slave example sketches.
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
void print_i2c_setup(void);
void print_i2c_status(void);
void test_rate(uint8_t target, uint32_t rate);

// Memory
#define MEM_LEN 256
uint8_t databuf[MEM_LEN];

void setup()
{
    pinMode(LED_BUILTIN,OUTPUT);    // LED
    digitalWrite(LED_BUILTIN,LOW);  // LED off
    pinMode(12,INPUT_PULLUP);       // Control for Test1
    pinMode(11,INPUT_PULLUP);       // Control for Test2
    pinMode(10,INPUT_PULLUP);       // Control for Test3
    pinMode(9,INPUT_PULLUP);        // Control for Test4
    pinMode(8,INPUT_PULLUP);        // Control for Test5

    Serial.begin(115200);
    
    // Setup for Master mode, pins 18/19, external pullups, 400kHz
    Wire.begin(I2C_MASTER, 0x00, I2C_PINS_18_19, I2C_PULLUP_EXT, 400000);

    Wire.setDefaultTimeout(250000); // 250ms default timeout
}

void loop()
{
    size_t addr, len;
    uint8_t target = 0x44; // slave addr
    uint32_t count;

    //
    // A sequence of different read/write techniques.
    // Pull respective control pin low to initiate sequence.
    //
    // All tests will first write values to the slave, then read back the values.
    // The readback values should match.
    //
    // The LED is turned on during I2C operations.  If it gets stuck on then the
    // ISR likely had a problem.  This can happen with excessive baud rate.
    //
    // Various forms of the Wire calls (blocking/non-blocking/STOP/NOSTOP) are
    // used in the different tests.
    //

    if(digitalRead(12) == LOW)
    {
        digitalWrite(LED_BUILTIN,HIGH);         // LED on
        Serial.print("---------------------------------------------------------\n");
        Serial.print("Test1 : Using blocking commands:\n");
        Serial.print("        1) WRITE memory in 32 byte blocks\n");
        Serial.print("        2) READ back memory in 32 byte blocks\n");
        Serial.print("---------------------------------------------------------\n");

        // Writing to Slave --------------------------------------------------------
        for(addr = 0; addr < 256; addr += 32)   // sweep addr in 32byte blocks
        {
            for(len = 0; len < 32; len++)       // prepare data to send
                databuf[len] = (addr+len)^0xFF; // set data (equal to bit inverse of memory address)

            Serial.printf("I2C WRITE 32 bytes to Slave 0x%0X at MemAddr %d\n", target, addr);
            Serial.print("Writing: ");
            for(len = 0; len < 32; len++) { Serial.printf("%d ",databuf[len]); }
            Serial.print("\n");

            Wire.beginTransmission(target);     // slave addr
            Wire.write(WRITE);                  // WRITE command
            Wire.write(addr);                   // memory address
            Wire.write(databuf, 32);            // write 32 byte block
            Wire.endTransmission();             // blocking write (when not specified I2C_STOP is implicit)

            print_i2c_status();                 // print I2C final status
        }

        // Reading from Slave ------------------------------------------------------
        for(addr = 0; addr < 256; addr += 32)   // sweep addr in 32byte blocks
        {
            Wire.beginTransmission(target);     // slave addr
            Wire.write(READ);                   // READ command
            Wire.write(addr);                   // memory address
            Wire.endTransmission(I2C_NOSTOP);   // blocking write (NOSTOP triggers RepSTART on next I2C command)
            Wire.requestFrom(target,32,I2C_STOP);// blocking read (request 32 bytes)

            Serial.printf("I2C READ 32 bytes from Slave 0x%0X at MemAddr %d\n", target, addr);
            Serial.print("Received: ");         // print received bytes
            while(Wire.available()) { Serial.printf("%d ", Wire.readByte()); }
            Serial.print("\n");
            print_i2c_status();                 // print I2C final status
        }

        digitalWrite(LED_BUILTIN,LOW);          // LED off
        delay(500);                             // delay to space out tests
    }


    if(digitalRead(11) == LOW)
    {
        digitalWrite(LED_BUILTIN,HIGH);         // LED on
        Serial.print("---------------------------------------------------------\n");
        Serial.print("Test2 : Using blocking commands:\n");
        Serial.print("        1) WRITE entire memory in a single 256 byte block\n");
        Serial.print("        2) READ back entire memory in a single 256 byte block\n");
        Serial.print("---------------------------------------------------------\n");

        // Writing to Slave --------------------------------------------------------
        addr = 0;
        for(len = 0; len < 256; len++)          // prepare data to send
            databuf[len] = (addr+len)^0xFF;     // set data (equal to bit inverse of memory address)

        Serial.printf("I2C WRITE 256 bytes to Slave 0x%0X at MemAddr %d\n", target, addr);
        Serial.print("Writing: ");
        for(len = 0; len < 256; len++) { Serial.printf("%d ",databuf[len]); }
        Serial.print("\n");

        Wire.beginTransmission(target);         // slave addr
        Wire.write(WRITE);                      // WRITE command
        Wire.write(addr);                       // memory address
        Wire.write(databuf, 256);               // write 256 byte block
        Wire.endTransmission(I2C_STOP);         // blocking write (using explicit I2C_STOP)

        print_i2c_status();                     // print I2C final status

        // Reading from Slave ------------------------------------------------------
        Wire.beginTransmission(target);         // slave addr
        Wire.write(READ);                       // READ command
        Wire.write(addr);                       // memory address
        Wire.endTransmission(I2C_NOSTOP);       // blocking write (NOSTOP triggers RepSTART on next I2C command)
        Wire.requestFrom(target,256,I2C_STOP);  // blocking read (request 256 bytes)

        Serial.printf("I2C READ %d bytes from Slave 0x%0X at MemAddr %d\n", Wire.available(), target, addr);
        Serial.print("Received: ");             // print received bytes
        while(Wire.available()) { Serial.printf("%d ", Wire.readByte()); }
        Serial.print("\n");
        print_i2c_status();                     // print I2C final status

        Serial.printf("Rate (Hz): %d\n", Wire.i2c->currentRate);
        
        digitalWrite(LED_BUILTIN,LOW);          // LED off
        delay(500);                             // delay to space out tests
    }


    if(digitalRead(10) == LOW)
    {
        digitalWrite(LED_BUILTIN,HIGH);         // LED on
        Serial.print("---------------------------------------------------------\n");
        Serial.print("Test3 : Using ISR NON-blocking commands:\n");
        Serial.print("        1) WRITE a 256 byte block to Slave.  While block is\n");
        Serial.print("           transferring, perform other commands.\n");
        Serial.print("        2) READ back the 256 byte block from Slave.  While\n");
        Serial.print("           block is transferring, perform other commands.\n");
        Serial.print("---------------------------------------------------------\n");

        // Set operating mode to ISR
        Wire.setOpMode(I2C_OP_MODE_ISR);

        // Writing to Slave --------------------------------------------------------
        addr = 0;
        for(len = 0; len < 256; len++)           // prepare data to send
            databuf[len] = (addr+len)^0xFF;     // set data (equal to bit inverse of memory address)

        Serial.printf("I2C WRITE 256 bytes to Slave 0x%0X at MemAddr %d\n", target, addr);
        Serial.print("Writing: ");
        for(len = 0; len < 256; len++) { Serial.printf("%d ",databuf[len]); }
        Serial.print("\n");

        Wire.beginTransmission(target);         // slave addr
        Wire.write(WRITE);                      // WRITE command
        Wire.write(addr);                       // memory address
        Wire.write(databuf, 256);               // write 256 byte block
        Wire.sendTransmission();                // NON-blocking write (when not specified I2C_STOP is implicit)

        Serial.print("...write sent, counting while waiting for Wire.done()...\n");
        count = 1;
        while(!Wire.done()) count++; // Since write is non-blocking, do some counting while waiting
        Serial.printf("Counted to: %d\n", count++);
        print_i2c_status();                     // print I2C final status

        // Reading from Slave ------------------------------------------------------
        Wire.beginTransmission(target);         // slave addr
        Wire.write(READ);                       // READ command
        Wire.write(addr);                       // memory address
        Wire.endTransmission(I2C_NOSTOP);       // blocking write (NOSTOP triggers RepSTART on next I2C command)
        Wire.sendRequest(target,256,I2C_STOP);  // NON-blocking read (request 256 bytes)

        // Since request is non-blocking, do some other things.
        Serial.print("...request sent, doing one thing then waiting for Wire.finish()...\n");

        // After doing something, use finish() to wait until I2C done
        Wire.finish();

        Serial.printf("I2C READ %d bytes from Slave 0x%0X at MemAddr %d\n", Wire.available(), target, addr);
        Serial.print("Received: ");             // print received bytes
        while(Wire.available()) { Serial.printf("%d ", Wire.readByte()); }
        Serial.print("\n");
        print_i2c_status();                     // print I2C final status

        digitalWrite(LED_BUILTIN,LOW);          // LED off
        delay(500);                             // delay to space out tests
    }


    if(digitalRead(9) == LOW)
    {
        digitalWrite(LED_BUILTIN,HIGH);         // LED on
        Serial.print("---------------------------------------------------------\n");
        Serial.print("Test4 : Using DMA NON-blocking commands:\n");
        Serial.print("        1) WRITE a 256 byte block to Slave.  While block is\n");
        Serial.print("           transferring, perform other commands.\n");
        Serial.print("        2) READ back the 256 byte block from Slave.  While\n");
        Serial.print("           block is transferring, perform other commands.\n");
        Serial.print("---------------------------------------------------------\n");

        // Set operating mode to DMA
        Serial.print("Trying to set DMA mode : ");
        Wire.setOpMode(I2C_OP_MODE_DMA);
        if(Wire.i2c->opMode == I2C_OP_MODE_DMA)
            Serial.printf("OK (Channel %d)\n",Wire.i2c->DMA->channel);
        else
            Serial.print("Failed, using ISR\n");

        // Writing to Slave --------------------------------------------------------
        addr = 0;
        for(len = 0; len < 256; len++)           // prepare data to send
            databuf[len] = (addr+len)^0xFF;     // set data (equal to bit inverse of memory address)

        Serial.printf("I2C WRITE 256 bytes to Slave 0x%0X at MemAddr %d\n", target, addr);
        Serial.print("Writing: ");
        for(len = 0; len < 256; len++) { Serial.printf("%d ",databuf[len]); }
        Serial.print("\n");

        Wire.beginTransmission(target);         // slave addr
        Wire.write(WRITE);                      // WRITE command
        Wire.write(addr);                       // memory address
        Wire.write(databuf, 256);               // write 256 byte block
        Wire.sendTransmission();                // NON-blocking write (when not specified I2C_STOP is implicit)

        Serial.print("...write sent, counting while waiting for Wire.done()...\n");
        count = 1;
        while(!Wire.done()) count++; // Since write is non-blocking, do some counting while waiting
        Serial.printf("Counted to: %d\n", count++);
        print_i2c_status();                     // print I2C final status

        // Reading from Slave ------------------------------------------------------
        Wire.beginTransmission(target);         // slave addr
        Wire.write(READ);                       // READ command
        Wire.write(addr);                       // memory address
        Wire.endTransmission(I2C_NOSTOP);       // blocking write (NOSTOP triggers RepSTART on next I2C command)
        Wire.sendRequest(target,256,I2C_STOP);  // NON-blocking read (request 256 bytes)

        // Since request is non-blocking, do some other things.
        Serial.print("...request sent, doing one thing then waiting for Wire.finish()...\n");

        // After doing something, use finish() to wait until I2C done
        Wire.finish();

        Serial.printf("I2C READ %d bytes from Slave 0x%0X at MemAddr %d\n", Wire.available(), target, addr);
        Serial.print("Received: ");             // print received bytes
        while(Wire.available()) { Serial.printf("%d ", Wire.readByte()); }
        Serial.print("\n");
        print_i2c_status();                     // print I2C final status

        digitalWrite(LED_BUILTIN,LOW);          // LED off
        delay(500);                             // delay to space out tests
    }


    if(digitalRead(8) == LOW)
    {
        uint8_t fail=0;
        
        digitalWrite(LED_BUILTIN,HIGH);         // LED on
        Serial.print("---------------------------------------------------------\n");
        Serial.print("Test5 : Rate adjustment tests.  This sweeps the I2C rates\n");
        Serial.print("        on both Master and Slave and times the duration \n");
        Serial.print("        of a 256 byte transfer at each rate.\n");
        Serial.print("---------------------------------------------------------\n");

        for(uint8_t opMode = I2C_OP_MODE_IMM; opMode <= I2C_OP_MODE_DMA; opMode++)
        {
            Wire.setOpMode((i2c_op_mode)opMode); // set op mode
            
            test_rate(target, 10000, fail);
            test_rate(target, 100000, fail);
            test_rate(target, 200000, fail);
            test_rate(target, 300000, fail);
            test_rate(target, 400000, fail);
            test_rate(target, 600000, fail);
            test_rate(target, 800000, fail);
            test_rate(target, 1000000, fail);
            test_rate(target, 1200000, fail);
            test_rate(target, 1500000, fail);
            test_rate(target, 1800000, fail);
            test_rate(target, 2000000, fail);
            test_rate(target, 2400000, fail);
            test_rate(target, 2800000, fail);
            test_rate(target, 3000000, fail);
            test_rate(target, 4000000, fail);
            test_rate(target, 5000000, fail);
            test_rate(target, 6000000, fail);

            // Restore normal settings

            // Change Slave rate
            Wire.beginTransmission(target);         // slave addr
            Wire.write(SETRATE);                    // SETRATE command
            Wire.write((uint8_t)400000&0xFF);       // rate LSB
            Wire.write((uint8_t)(400000>>8)&0xFF);
            Wire.write((uint8_t)(400000>>16)&0xFF);
            Wire.write((uint8_t)(400000>>24)&0xFF); // rate MSB
            Wire.endTransmission();                 // blocking write

            // Change Master rate
            Wire.setClock(400000);

            fail = 0;  // reset flag
        }
        Wire.setOpMode(I2C_OP_MODE_ISR);        // restore default ISR mode

        print_i2c_status();                     // print I2C final status

        // Restore normal settings (400kHz)

        // Change Slave rate
        Wire.beginTransmission(target);         // slave addr
        Wire.write(SETRATE);                    // SETRATE command
        Wire.write((uint8_t)400000&0xFF);       // rate LSB
        Wire.write((uint8_t)(400000>>8)&0xFF);
        Wire.write((uint8_t)(400000>>16)&0xFF);
        Wire.write((uint8_t)(400000>>24)&0xFF); // rate MSB
        Wire.endTransmission();                 // blocking write

        // Change Master rate
        Wire.setClock(400000);

        digitalWrite(LED_BUILTIN,LOW);          // LED off
        delay(500);                             // delay to space out tests
    }
}


//
// print current setup
//
void print_i2c_setup()
{
    Serial.print("Mode:");
    switch(Wire.i2c->opMode)
    {
    case I2C_OP_MODE_IMM: Serial.print("IMM    "); break;
    case I2C_OP_MODE_ISR: Serial.print("ISR    "); break;
    case I2C_OP_MODE_DMA: Serial.printf("DMA[%d] ",Wire.i2c->DMA->channel); break;
    }
    Serial.printf("Pins: %d/%d ", Wire.i2c->currentSCL, Wire.i2c->currentSDA);
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


//
// print I2C rate
//
void print_rate(uint32_t rate)
{
    Serial.printf("%d Hz    ", rate);
}


//
// test rate
//
void test_rate(uint8_t target, uint32_t rate, uint8_t& fail)
{
    uint32_t deltatime=0;
    size_t len;
    
    if(!fail)
    {
        for(len = 0; len < 256; len++)  // prepare data to send
            databuf[len] = len;         // set data (equal to addr)
    
        // Change Slave rate
        Wire.beginTransmission(target);       // slave addr
        Wire.write(SETRATE);                  // SETRATE command
        Wire.write((uint8_t)rate&0xFF);       // rate LSB
        Wire.write((uint8_t)(rate>>8)&0xFF);
        Wire.write((uint8_t)(rate>>16)&0xFF);
        Wire.write((uint8_t)(rate>>24)&0xFF); // rate MSB
        Wire.endTransmission();               // blocking write
        fail = Wire.getError();
    
        if(!fail)
        {
            // Change Master rate
            Wire.setClock(rate);
        
            // Setup write buffer
            Wire.beginTransmission(target); // slave addr
            Wire.write(WRITE);              // WRITE command
            Wire.write(0);                  // memory address
            Wire.write(databuf, 256);       // write 256 byte block
        
            // Write to Slave
            elapsedMicros deltaT;
            Wire.endTransmission();         // blocking write
            deltatime = deltaT;
            fail = Wire.getError();
        
            if(!fail)
            {
                Wire.beginTransmission(target);     // slave addr
                Wire.write(READ);                   // READ command
                Wire.write(0);                      // memory address
                Wire.endTransmission(I2C_NOSTOP);   // blocking write (NOSTOP triggers RepSTART on next I2C command)
                Wire.requestFrom(target,256,I2C_STOP);// blocking read
                fail = Wire.getError();
        
                if(!fail)
                {
                    for(len = 0; len < 256; len++)  // verify block
                        if(databuf[len] != Wire.readByte()) { fail=1; break; }
                }
            }
        }

        print_i2c_setup();
        if(!fail)
        {
            // Print result
            Serial.print("256 byte transfer at ");
            print_rate(rate);
            Serial.printf(" (Actual Rate (Hz): %d) : %d us : ", Wire.getClock(), deltatime);
            print_i2c_status();
        }
        else
        {
            Serial.printf("Transfer fail : %d us : ",deltatime);
            print_i2c_status();
        }
    }
}
