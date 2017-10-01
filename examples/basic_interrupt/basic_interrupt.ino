// -------------------------------------------------------------------------------------------
// Interrupt
// -------------------------------------------------------------------------------------------
//
// This creates an I2C master device which will issue I2C commands from inside a periodic
// interrupt (eg. similar to reading a sensor at regular time intervals).  For this test the 
// Slave device will be assumed to be that given in the basic_slave sketch.
//
// The test will start when the Serial monitor opens.
//
// This example code is in the public domain.
//
// -------------------------------------------------------------------------------------------

#include <i2c_t3.h>

// Function prototypes
void rwSlave(void);

// Timer
IntervalTimer slaveTimer;

// Memory
#define MEM_LEN 32
char databuf[MEM_LEN];
size_t count=0;
uint8_t target=0x66;

void setup()
{
    pinMode(LED_BUILTIN,OUTPUT);        // LED

    // Setup for Master mode, pins 18/19, external pullups, 400kHz, 10ms default timeout
    Wire.begin(I2C_MASTER, 0x00, I2C_PINS_18_19, I2C_PULLUP_EXT, 400000);
    Wire.setDefaultTimeout(10000);

    // Data init
    memset(databuf, 0, sizeof(databuf));
    count = 0;

    // setup Serial and wait for monitor to start
    Serial.begin(115200);
    while(!Serial);

    // Start reading Slave device
    slaveTimer.begin(rwSlave,1000000); // 1s timer
}

void loop()
{
    delay(1);
}

void rwSlave(void)
{
    digitalWrite(LED_BUILTIN,HIGH);         // pulse LED when reading

    // Construct data message
    sprintf(databuf, "Msg #%d", count++);
    
    // Transmit to Slave
    Wire.beginTransmission(target);         // Slave address
    Wire.write(databuf,strlen(databuf)+1);  // Write string to I2C Tx buffer (incl. string null at end)
    Wire.endTransmission();                 // Transmit to Slave

    // Check if error occured
    if(Wire.getError())
        Serial.print("Write FAIL\n");
    else
    {
        // Read from Slave
        Wire.requestFrom(target, (size_t)MEM_LEN); // Read from Slave (string len unknown, request full buffer)

        // Check if error occured
        if(Wire.getError())
            Serial.print("Read FAIL\n");
        else
        {
            // If no error then read Rx data into buffer and print
            Wire.read(databuf, Wire.available());
            Serial.printf("'%s' OK\n",databuf);
        }
    }

    digitalWrite(LED_BUILTIN,LOW);
}
