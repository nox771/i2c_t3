// -------------------------------------------------------------------------------------------
// Loopback
// -------------------------------------------------------------------------------------------
//
// This creates a device using one I2C bus as a Master and one or more I2C buses as Slaves.
// When the buses are connected together in closed loopback, then it creates a closed test
// environment which allows Master/Slave development on a single device.  
//
// Wire will act as the Master device, and Wire1/2/3 will act as the Slave device(s).
//
// Pulling pin12 low will initiate a WRITE then READ transfer between Master and Slave(s).
//
// This example code is in the public domain.
//
// -------------------------------------------------------------------------------------------

#include <i2c_t3.h>


// -------------------------------------------------------------------------------------------
// Defines - modify as needed for pin config
//
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


// Function prototypes and variables
//
void rwSlave(uint8_t target);
void printWireStatus(void);
void printStatus(i2c_status status);
#define MEM_LEN 16
uint8_t mem[MEM_LEN];

#if I2C_BUS_NUM >= 2
void receiveEvent1(size_t len);
void requestEvent1(void);
void printWire1Status(void);
uint8_t mem1[MEM_LEN];
#define TARGET1 0x1A
#endif

#if I2C_BUS_NUM >= 3
void receiveEvent2(size_t len);
void requestEvent2(void);
void printWire2Status(void);
uint8_t mem2[MEM_LEN];
#define TARGET2 0x2B
#endif

#if I2C_BUS_NUM >= 4
void receiveEvent3(size_t len);
void requestEvent3(void);
void printWire3Status(void);
uint8_t mem3[MEM_LEN];
#define TARGET3 0x3C
#endif


void setup()
{
    pinMode(LED_BUILTIN,OUTPUT);    // LED
    pinMode(12,INPUT_PULLUP);       // Control for Test1

    // Wire - Setup for Master mode, external pullup, 400kHz, 200ms default timeout
    Wire.begin(I2C_MASTER, 0x00, WIRE_PINS, I2C_PULLUP_EXT, 400000);
    Wire.setDefaultTimeout(200000);
    memset(mem, 0, sizeof(mem));

    #if I2C_BUS_NUM >= 2
    // Wire1 - Setup for Slave mode, external pullup, 400kHz
    Wire1.begin(I2C_SLAVE, TARGET1, WIRE1_PINS, I2C_PULLUP_EXT, 400000);
    Wire1.onReceive(receiveEvent1);
    Wire1.onRequest(requestEvent1);
    memset(mem1, 0, sizeof(mem1));
    #endif
    
    #if I2C_BUS_NUM >= 3
    // Wire2 - Setup for Slave mode, external pullup, 400kHz
    Wire2.begin(I2C_SLAVE, TARGET2, WIRE2_PINS, I2C_PULLUP_EXT, 400000);
    Wire2.onReceive(receiveEvent2);
    Wire2.onRequest(requestEvent2);
    memset(mem2, 0, sizeof(mem2));
    #endif
    
    #if I2C_BUS_NUM >= 4
    // Wire3 - Setup for Slave mode, external pullup, 400kHz
    Wire3.begin(I2C_SLAVE, TARGET3, WIRE3_PINS, I2C_PULLUP_EXT, 400000);
    Wire3.onReceive(receiveEvent3);
    Wire3.onRequest(requestEvent3);
    memset(mem3, 0, sizeof(mem3));
    #endif

    Serial.begin(115200);
}


//
// Master Loop
//
void loop()
{
    if(digitalRead(12) == LOW)
    {      
        Serial.print("------------------------------------------------------------\n");
        Serial.printf("Writing then reading a %d byte block from Slave device(s)\n", MEM_LEN);
        Serial.print("------------------------------------------------------------\n");

        digitalWrite(LED_BUILTIN,HIGH);         // LED on

        #if I2C_BUS_NUM >= 2
        rwSlave(TARGET1);
        #endif
        #if I2C_BUS_NUM >= 3
        rwSlave(TARGET2);
        #endif
        #if I2C_BUS_NUM >= 4
        rwSlave(TARGET3);
        #endif
        
        delay(100);                             // delay to space out tests
        digitalWrite(LED_BUILTIN,LOW);          // LED off
    }
}


//
// Write/Read to Slave
//
void rwSlave(uint8_t target)
{
    uint8_t fail=0, count, data;
    
    // Writing to Slave ------------------------------------------------------
    Wire.beginTransmission(target);         // slave addr
    Serial.printf("I2C WRITE %d bytes   to Slave 0x%0X : ", MEM_LEN, target);
    for(count = 0; count < MEM_LEN; count++) // prepare data to send
    {
        mem[count] = random(0xFF);          // set random data
        Serial.printf("%02X ",mem[count]);
        Wire.write(mem[count]);
    }
    fail = Wire.endTransmission();          // blocking write
    Serial.print("       ");
    printWireStatus();                      // print I2C final status

    // Reading from Slave ------------------------------------------------------
    if(!fail)
    {
        Wire.requestFrom(target,(size_t)MEM_LEN);  // blocking read
        fail = Wire.getError();

        Serial.printf("I2C  READ %d bytes from Slave 0x%0X : ", Wire.available(), target);
        for(count = 0; count < MEM_LEN && Wire.available(); count++) // verify block
        {
            data = Wire.readByte();
            Serial.printf("%02X ", data);
            if(mem[count] != data) fail++;
        }
        if(!fail)
            Serial.print("  OK   ");
        else
            Serial.print(" FAIL  ");
        printWireStatus();                  // print I2C final status
    }
}


//
// Slave ISR handlers
//
#if I2C_BUS_NUM >= 2
void receiveEvent1(size_t count)  // Handle Wire1 Slave Rx Event (incoming I2C request/data)
{
    Wire1.read(mem1, count);      // copy Rx data to databuf
}
void requestEvent1(void)          // Handle Wire1 Tx Event (outgoing I2C data)
{
    Wire1.write(mem1, MEM_LEN);   // fill Tx buffer (send full mem)
}
#endif

#if I2C_BUS_NUM >= 3
void receiveEvent2(size_t count)  // Handle Wire2 Slave Rx Event (incoming I2C request/data)
{
    Wire2.read(mem2, count);      // copy Rx data to databuf
}
void requestEvent2(void)          // Handle Wire2 Tx Event (outgoing I2C data)
{
    Wire2.write(mem2, MEM_LEN);   // fill Tx buffer (send full mem)
}
#endif

#if I2C_BUS_NUM >= 4
void receiveEvent3(size_t count)  // Handle Wire3 Slave Rx Event (incoming I2C request/data)
{
    Wire3.read(mem3, count);      // copy Rx data to databuf
}
void requestEvent3(void)          // Handle Wire3 Tx Event (outgoing I2C data)
{
    Wire3.write(mem3, MEM_LEN);   // fill Tx buffer (send full mem)
}
#endif


//
// print I2C status
//
void printWireStatus(void) { printStatus(Wire.status()); }
#if I2C_BUS_NUM >= 2
void printWire1Status(void) { printStatus(Wire1.status()); }
#endif
#if I2C_BUS_NUM >= 3
void printWire2Status(void) { printStatus(Wire2.status()); }
#endif
#if I2C_BUS_NUM >= 4
void printWire3Status(void) { printStatus(Wire3.status()); }
#endif
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

