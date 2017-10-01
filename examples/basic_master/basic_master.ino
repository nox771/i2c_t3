// -------------------------------------------------------------------------------------------
// Basic Master
// -------------------------------------------------------------------------------------------
//
// This creates a simple I2C Master device which when triggered will send/receive a text 
// string to/from a Slave device.  It is intended to pair with a Slave device running the 
// basic_slave sketch.
//
// Pull pin12 input low to send.
// Pull pin11 input low to receive.
//
// This example code is in the public domain.
//
// -------------------------------------------------------------------------------------------

#include <i2c_t3.h>

// Memory
#define MEM_LEN 256
char databuf[MEM_LEN];
int count;

void setup()
{
    pinMode(LED_BUILTIN,OUTPUT);    // LED
    digitalWrite(LED_BUILTIN,LOW);  // LED off
    pinMode(12,INPUT_PULLUP);       // Control for Send
    pinMode(11,INPUT_PULLUP);       // Control for Receive

    // Setup for Master mode, pins 18/19, external pullups, 400kHz, 200ms default timeout
    Wire.begin(I2C_MASTER, 0x00, I2C_PINS_18_19, I2C_PULLUP_EXT, 400000);
    Wire.setDefaultTimeout(200000); // 200ms

    // Data init
    memset(databuf, 0, sizeof(databuf));
    count = 0;

    Serial.begin(115200);
}

void loop()
{
    uint8_t target = 0x66; // target Slave address
 
    // Send string to Slave
    //
    if(digitalRead(12) == LOW)
    {
        digitalWrite(LED_BUILTIN,HIGH);   // LED on

        // Construct data message
        sprintf(databuf, "Data Message #%d", count++);

        // Print message
        Serial.printf("Sending to Slave: '%s' ", databuf);
        
        // Transmit to Slave
        Wire.beginTransmission(target);   // Slave address
        Wire.write(databuf,strlen(databuf)+1); // Write string to I2C Tx buffer (incl. string null at end)
        Wire.endTransmission();           // Transmit to Slave

        // Check if error occured
        if(Wire.getError())
            Serial.print("FAIL\n");
        else
            Serial.print("OK\n");

        digitalWrite(LED_BUILTIN,LOW);    // LED off
        delay(100);                       // Delay to space out tests
    }

    // Read string from Slave
    //
    if(digitalRead(11) == LOW)
    {
        digitalWrite(LED_BUILTIN,HIGH);   // LED on

        // Print message
        Serial.print("Reading from Slave: ");
        
        // Read from Slave
        Wire.requestFrom(target, (size_t)MEM_LEN); // Read from Slave (string len unknown, request full buffer)

        // Check if error occured
        if(Wire.getError())
            Serial.print("FAIL\n");
        else
        {
            // If no error then read Rx data into buffer and print
            Wire.read(databuf, Wire.available());
            Serial.printf("'%s' OK\n",databuf);
        }

        digitalWrite(LED_BUILTIN,LOW);    // LED off
        delay(100);                       // Delay to space out tests
    }
}

