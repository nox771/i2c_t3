// -------------------------------------------------------------------------------------------
// Basic Master Multiplexed
// -------------------------------------------------------------------------------------------
//
// This creates an I2C multiplexed master device which talks to the simple I2C slave device
// given in the basic_slave sketch.  It is different than the basic_master sketch because it can
// output I2C commands on either pins 18/19 or pins 16/17 (both using the same I2C bus - Wire), 
// and change pins on-the-fly.
//
// The purpose of this sketch is to demonstrate how the pins can be reconfigured on-the-fly 
// when the bus is idle.
//
// Pull pin12 low to send command to slave on bus1 (pins 16/17)
// Pull pin11 low to send command to slave on bus2 (pins 18/19)
//
// This example code is in the public domain.
//
// -------------------------------------------------------------------------------------------

#include <i2c_t3.h>

// Memory
#define MEM_LEN 256
char databuf[MEM_LEN];
int count;
uint8_t pin12, pin11, target = 0x66; // target Slave address
size_t idx;

void setup()
{
    pinMode(LED_BUILTIN,OUTPUT);    // LED
    digitalWrite(LED_BUILTIN,LOW);  // LED off
    pinMode(12,INPUT_PULLUP);       // Control for Test1
    pinMode(11,INPUT_PULLUP);       // Control for Test2

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
    // Send string to Slave
    //
    pin12 = (digitalRead(12) == LOW); // latch pin values
    pin11 = (digitalRead(11) == LOW);
    if(pin12 || pin11)
    {
        // Construct data message
        sprintf(databuf, "Data Message #%d", count++);

        // Change pins
        if(pin12)
        {
            Wire.pinConfigure(I2C_PINS_16_17, I2C_PULLUP_EXT);
            Serial.printf("Sending to Slave 0x%02X on pins 16/17: '%s' ", target, databuf);
        }
        else
        {
            Wire.pinConfigure(I2C_PINS_18_19, I2C_PULLUP_EXT);
            Serial.printf("Sending to Slave 0x%02X on pins 18/19: '%s' ", target, databuf);
        }
        
        digitalWrite(LED_BUILTIN,HIGH);   // LED on
        
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
}

