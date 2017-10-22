// -------------------------------------------------------------------------------------------
// I2C Advanced Bus Scanner
// -------------------------------------------------------------------------------------------
//
// This creates an I2C master device which will scan the address space and report all
// devices which ACK.  It does not attempt to transfer data, it only reports which devices
// ACK their address.
//
// This version will sweep all existing I2C buses (eg. Wire, Wire1, Wire2, Wire3).
//
// Pull the control pin low to initiate the scan.  Result will output to Serial.
//
// This example code is in the public domain.
// -------------------------------------------------------------------------------------------

#include <i2c_t3.h>

// -------------------------------------------------------------------------------------------
// Defines - modify as needed for sweep range and bus pin config
//
#define TARGET_START 0x01
#define TARGET_END   0x7F

#define WIRE_PINS   I2C_PINS_18_19
#if defined(__MKL26Z64__)               // LC
#define WIRE1_PINS   I2C_PINS_22_23
#endif
#if defined(__MK20DX256__)              // 3.1-3.2
#define WIRE1_PINS   I2C_PINS_29_30
#endif
#if defined(__MK64FX512__) || defined(__MK66FX1M0__)  // 3.5/3.6
#define WIRE1_PINS   I2C_PINS_37_38
#define WIRE2_PINS   I2C_PINS_3_4
#endif
#if defined(__MK66FX1M0__)              // 3.6
#define WIRE3_PINS   I2C_PINS_56_57
#endif

// -------------------------------------------------------------------------------------------
// Function prototypes
void scan_bus(i2c_t3& Wire, uint8_t all);
void print_bus_status(i2c_t3& Wire);
void print_scan_status(struct i2cStruct* i2c, uint8_t target, uint8_t& found, uint8_t all);

// -------------------------------------------------------------------------------------------
void setup()
{
    pinMode(LED_BUILTIN,OUTPUT);    // LED
    pinMode(12,INPUT_PULLUP);       // pull pin 12 low to show ACK only results
    pinMode(11,INPUT_PULLUP);       // pull pin 11 low for a more verbose result (shows both ACK and NACK)

    // Setup for Master mode, all buses, external pullups, 400kHz, 10ms default timeout
    //
    Wire.begin(I2C_MASTER, 0x00, WIRE_PINS, I2C_PULLUP_EXT, 400000);
    Wire.setDefaultTimeout(10000); // 10ms
    #if I2C_BUS_NUM >= 2
    Wire1.begin(I2C_MASTER, 0x00, WIRE1_PINS, I2C_PULLUP_EXT, 400000);
    Wire1.setDefaultTimeout(10000); // 10ms
    #endif
    #if I2C_BUS_NUM >= 3
    Wire2.begin(I2C_MASTER, 0x00, WIRE2_PINS, I2C_PULLUP_EXT, 400000);
    Wire2.setDefaultTimeout(10000); // 10ms
    #endif
    #if I2C_BUS_NUM >= 4
    Wire3.begin(I2C_MASTER, 0x00, WIRE3_PINS, I2C_PULLUP_EXT, 400000);
    Wire3.setDefaultTimeout(10000); // 10ms
    #endif

    Serial.begin(115200);
}

// -------------------------------------------------------------------------------------------
void loop()
{
    // Scan I2C addresses
    //
    if(digitalRead(12) == LOW || digitalRead(11) == LOW)
    {
        uint8_t all = (digitalRead(11) == LOW);

        Serial.print("---------------------------------------------------\n");
        Serial.print("Bus Status Summary\n");
        Serial.print("==================\n");
        Serial.print(" Bus    Mode   SCL  SDA   Pullup   Clock\n");
        print_bus_status(Wire);
        #if I2C_BUS_NUM >= 2
        print_bus_status(Wire1);
        #endif
        #if I2C_BUS_NUM >= 3
        print_bus_status(Wire2);
        #endif
        #if I2C_BUS_NUM >= 4
        print_bus_status(Wire3);
        #endif

        scan_bus(Wire, all);
        #if I2C_BUS_NUM >= 2
        scan_bus(Wire1, all);
        #endif
        #if I2C_BUS_NUM >= 3
        scan_bus(Wire2, all);
        #endif
        #if I2C_BUS_NUM >= 4
        scan_bus(Wire3, all);
        #endif

        Serial.print("---------------------------------------------------\n\n\n");

        delay(500); // delay to space out tests
    }
}

// -------------------------------------------------------------------------------------------
// scan bus
//
void scan_bus(i2c_t3& Wire, uint8_t all)
{
    uint8_t target, found = 0;
    
    Serial.print("---------------------------------------------------\n");
    if(Wire.bus == 0)
        Serial.print("Starting scan: Wire\n");
    else
        Serial.printf("Starting scan: Wire%d\n",Wire.bus);
    
    digitalWrite(LED_BUILTIN,HIGH); // LED on
    for(target = TARGET_START; target <= TARGET_END; target++) // sweep addr, skip general call
    {
        Wire.beginTransmission(target);       // slave addr
        Wire.endTransmission();               // no data, just addr
        print_scan_status(Wire.i2c, target, found, all);
    }    
    digitalWrite(LED_BUILTIN,LOW); // LED off
    
    if(!found) Serial.print("No devices found.\n");
}

// -------------------------------------------------------------------------------------------
// print bus status
//
void print_bus_status(i2c_t3& Wire)
{
    struct i2cStruct* i2c = Wire.i2c;
    if(Wire.bus == 0)
        Serial.print("Wire   ");
    else
        Serial.printf("Wire%d  ",Wire.bus);
    switch(i2c->currentMode)
    {
    case I2C_MASTER: Serial.print("MASTER  "); break;
    case I2C_SLAVE:  Serial.print(" SLAVE  "); break;
    }
    Serial.printf(" %2d   %2d  ", Wire.i2c->currentSCL, Wire.i2c->currentSDA);
    switch(i2c->currentPullup)
    {
    case I2C_PULLUP_EXT: Serial.print("External  "); break;
    case I2C_PULLUP_INT: Serial.print("Internal  "); break;
    }
    Serial.printf("%d Hz\n",i2c->currentRate);
}

// -------------------------------------------------------------------------------------------
// print scan status
//
void print_scan_status(struct i2cStruct* i2c, uint8_t target, uint8_t& found, uint8_t all)
{
    switch(i2c->currentStatus)
    {
    case I2C_WAITING:  Serial.printf("Addr: 0x%02X ACK\n",target); found=1; break;
    case I2C_ADDR_NAK: if(all) { Serial.printf("Addr: 0x%02X\n",target); } break;
    case I2C_TIMEOUT:  if(all) { Serial.printf("Addr: 0x%02X Timeout\n",target); } break;
    default: break;
    }
}

