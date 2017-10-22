// -------------------------------------------------------------------------------------------
// I2C Bus Scanner
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
void print_scan_status(uint8_t target, uint8_t all);

uint8_t found, target, all;

void setup()
{
    pinMode(LED_BUILTIN,OUTPUT);    // LED
    pinMode(12,INPUT_PULLUP);       // pull pin 12 low to show ACK only results
    pinMode(11,INPUT_PULLUP);       // pull pin 11 low for a more verbose result (shows both ACK and NACK)

    // Setup for Master mode, pins 18/19, external pullups, 400kHz, 10ms default timeout
    Wire.begin(I2C_MASTER, 0x00, I2C_PINS_18_19, I2C_PULLUP_EXT, 400000);
    Wire.setDefaultTimeout(10000); // 10ms

    Serial.begin(115200);
}

void loop()
{
    // Scan I2C addresses
    //
    if(digitalRead(12) == LOW || digitalRead(11) == LOW)
    {
        all = (digitalRead(11) == LOW);
        found = 0;
        
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

        if(!found) Serial.print("No devices found.\n");
        
        delay(500); // delay to space out tests
    }
}

//
// print scan status
//
void print_scan_status(uint8_t target, uint8_t all)
{
    switch(Wire.status())
    {
    case I2C_WAITING:  
        Serial.printf("Addr: 0x%02X ACK\n", target);
        found = 1;
        break;
    case I2C_ADDR_NAK: 
        if(all) Serial.printf("Addr: 0x%02X\n", target); 
        break;
    default:
        break;
    }
}

