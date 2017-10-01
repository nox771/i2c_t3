// -------------------------------------------------------------------------------------------
// Basic Slave with Address Range
// -------------------------------------------------------------------------------------------
//
// This creates a simple I2C Slave device which will print whatever text string is sent to it.
// It will retain the text string in memory and will send it back to a Master device if 
// requested.  It is intended to pair with a Master device running the basic_master sketch.
//
// The Slave will respond to a range of I2C addresses.  It demonstrates use of the getRxAddr() 
// function which can be used to obtain the Slave address.  This allows a single device to 
// act as multiple different Slave targets.
//
// This example code is in the public domain.
//
// -------------------------------------------------------------------------------------------

#include <i2c_t3.h>

// Function prototypes
void receiveEvent(size_t len);
void requestEvent(void);

// Memory
#define MEM_LEN 256
uint8_t databuf[MEM_LEN];
volatile uint8_t target;
volatile uint8_t received;

//
// Setup
//
void setup()
{
    pinMode(LED_BUILTIN,OUTPUT); // LED

    // Setup for Slave mode, addresses 0x08 to 0x77, pins 18/19, external pullups, 400kHz
    Wire.begin(I2C_SLAVE, 0x08, 0x77, I2C_PINS_18_19, I2C_PULLUP_EXT, 400000);

    // Data init
    received = 0;
    target = 0;
    memset(databuf, 0, sizeof(databuf));

    // register events
    Wire.onReceive(receiveEvent);
    Wire.onRequest(requestEvent);

    Serial.begin(115200);
}

void loop()
{
    // print received data
    if(received)
    {
        digitalWrite(LED_BUILTIN,HIGH);
        Serial.printf("Slave 0x%02X received: '%s'\n", target, (char*)databuf);
        received = 0;
        digitalWrite(LED_BUILTIN,LOW);
    }
}

//
// handle Rx Event (incoming I2C data)
//
void receiveEvent(size_t count)
{
    target = Wire.getRxAddr();  // getRxAddr() is used to obtain Slave address
    Wire.read(databuf, count);  // copy Rx data to databuf
    received = count;           // set received flag to count, this triggers print in main loop
}

//
// handle Tx Event (outgoing I2C data)
//
void requestEvent(void)
{
    Wire.write(databuf, MEM_LEN); // fill Tx buffer (send full mem)
}

