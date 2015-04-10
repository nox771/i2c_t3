// -------------------------------------------------------------------------------------------
// Teensy3.0/3.1/LC I2C Scanner
// 08Mar13 Brian (nox771 at gmail.com)
// -------------------------------------------------------------------------------------------
//
// This creates an I2C master device which will scan the address space and report all
// devices which ACK.  It does not attempt to transfer data, it only reports which devices
// ACK their address.
//
// Pull the control pin low to initiate the scan.  Result will output to Serial.
//
// This example code is in the public domain.
// -------------------------------------------------------------------------------------------

#include <i2c_t3.h>

// Function prototypes
void print_scan_status(uint8_t target, bool all);

void setup()
{
    pinMode(LED_BUILTIN,OUTPUT);    // LED
    pinMode(12,INPUT_PULLUP);       // Control for Test1
    pinMode(11,INPUT_PULLUP);       // Control for Test2

    Serial.begin(115200);

    // Setup for Master mode, pins 18/19, external pullups, 400kHz
    Wire.begin(I2C_MASTER, 0x00, I2C_PINS_18_19, I2C_PULLUP_EXT, I2C_RATE_400);
}

void loop()
{
    uint8_t target; // slave addr
    bool all;

    //
    // Test1/2 - scan I2C addresses
    //         - pull pin 12 low to show ACK only results
    //         - pull pin 11 low for a little more verbose result (shows both ACK and NACK)
    //
    if(digitalRead(12) == LOW || digitalRead(11) == LOW)
    {
        all = (digitalRead(11) == LOW);
        Serial.print("---------------------------------------------------\n");
        Serial.print("Starting scan...\n");
        digitalWrite(LED_BUILTIN,HIGH); // LED on
        for(target = 1; target <= 0x7F; target++) // sweep addr, skip general call
        {
            Wire.beginTransmission(target);       // slave addr
            Wire.endTransmission();               // no data, just addr
            print_scan_status(target, all);
        }
        digitalWrite(LED_BUILTIN,LOW); // LED off
        Serial.print("---------------------------------------------------\n");

        delay(500); // delay to space out tests
    }
}

//
// print scan status
//
void print_scan_status(uint8_t target, bool all)
{
    switch(Wire.status())
    {
    case I2C_WAITING:
        Serial.print("Addr 0x");
        Serial.print(target,HEX);
        Serial.print(" ACK\n");
        break;
    case I2C_ADDR_NAK:
        if(all)
        {
            Serial.print("Addr 0x");
            Serial.print(target,HEX);
            Serial.print("\n");
        }
        break;
    default:
        break;
    }
}
