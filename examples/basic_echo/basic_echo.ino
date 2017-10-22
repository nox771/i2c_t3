// -------------------------------------------------------------------------------------------
// Basic Echo
// -------------------------------------------------------------------------------------------
//
// This creates a simple I2C Slave device which will read whatever data is sent to it 
// on Wire1 and output that same data on Wire.  It will retain the data in memory and will 
// send it back to a Master device if requested. 
//
// This code demonstrates non-blocking nested operation - calling Wire in a non-blocking way
// inside of Wire1 receive ISR.
//
// Wire1 bus is intended to pair with a Master device running the basic_master sketch.
// Wire bus is intended to pair with a Slave device running the basic_slave sketch.
//
// This code is setup for a Teensy 3.5/3.6 device.
//
// This example code is in the public domain.
//
// -------------------------------------------------------------------------------------------

#include <i2c_t3.h>

// Function prototypes
void receiveEvent(size_t count);
void requestEvent(void);

// Memory
#define MEM_LEN 256
uint8_t databuf[MEM_LEN];
volatile uint8_t received;

//
// Setup
//
void setup()
{
    pinMode(LED_BUILTIN,OUTPUT); // LED

    // Setup for Master mode, pins 18/19, external pullups, 400kHz, 200ms default timeout
    Wire.begin(I2C_MASTER, 0x00, I2C_PINS_18_19, I2C_PULLUP_EXT, 400000);
    Wire.setDefaultTimeout(200000); // 200ms

    // Setup for Slave mode, address 0x66, pins 37/38, external pullups, 400kHz
    Wire1.begin(I2C_SLAVE, 0x66, I2C_PINS_37_38, I2C_PULLUP_EXT, 400000);   // 3.5/3.6

    // Data init
    received = 0;
    memset(databuf, 0, sizeof(databuf));

    // register events
    Wire1.onReceive(receiveEvent);
    Wire1.onRequest(requestEvent);

    Serial.begin(115200);
}

void loop()
{
    // print received data - this is done in main loop to keep time spent in I2C ISR to minimum
    if(received)
    {
        digitalWrite(LED_BUILTIN,HIGH);
        Serial.printf("Echoing data received: '%s'\n", databuf);
        received = 0;
        digitalWrite(LED_BUILTIN,LOW);
    }
}

//
// handle Rx Event (incoming I2C data)
//
void receiveEvent(size_t count)
{
    digitalWrite(LED_BUILTIN,HIGH); // LED on
    
    Wire1.read(databuf, count);     // copy Rx data to databuf
    
    Wire.finish();                  // finish any prev non-blocking Tx
    Wire.beginTransmission(0x66);   // init for Tx send  
    Wire.write(databuf, count);     // send to Tx buffer
    Wire.sendTransmission();        // Send Tx data to Slave (non-blocking)
    
    received = count;               // set received flag to count, this triggers print in main loop

    digitalWrite(LED_BUILTIN,LOW);  // LED off
}

//
// handle Tx Event (outgoing I2C data)
//
void requestEvent(void)
{
    Wire1.write(databuf, MEM_LEN); // fill Tx buffer (send full mem)
}

