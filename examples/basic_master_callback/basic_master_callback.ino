// -------------------------------------------------------------------------------------------
// Basic Master Callback
// -------------------------------------------------------------------------------------------
//
// This creates a simple I2C Master device which when triggered will send/receive a text
// string to/from a Slave device.  It is intended to pair with a Slave device running the
// basic_slave sketch.  
//
// Functionally this sketch is similar to the basic_master sketch, except for three things:
// 1) It uses callbacks to handle the results and errors, instead of explicitly coding such
//    routines after each send/receive call.
// 2) It can optionally demonstrate background transfers by setting the NONBLOCKING flag (from
//    external observation these transfers will appear similar, but the LED duration will
//    be shorter as they exit immediately after initiating the transfer).
// 3) There is an added test to dump error diagnostics from the error counting system.
//
// Pull pin12 input low to send.
// Pull pin11 input low to receive.
// Pull pin10 input low to dump error diagnostics.
//
// This example code is in the public domain.
//
// -------------------------------------------------------------------------------------------

#include <i2c_t3.h>

// Memory
#define MEM_LEN 256
char databuf[MEM_LEN];
int count;

#define NONBLOCKING

void setup()
{
    pinMode(LED_BUILTIN,OUTPUT);    // LED
    digitalWrite(LED_BUILTIN,LOW);  // LED off
    pinMode(12,INPUT_PULLUP);       // Control for Send
    pinMode(11,INPUT_PULLUP);       // Control for Receive
    pinMode(10,INPUT_PULLUP);       // Control for Diagnostics

    // Setup for Master mode, pins 18/19, external pullups, 400kHz, 50ms default timeout
    Wire.begin(I2C_MASTER, 0x00, I2C_PINS_18_19, I2C_PULLUP_EXT, 400000);
    Wire.setDefaultTimeout(50000); // 50ms

    // Data init
    memset(databuf, 0, sizeof(databuf));
    count = 0;

    Serial.begin(115200);

    Wire.onTransmitDone(transmitDone);
    Wire.onReqFromDone(requestDone);
    Wire.onError(errorEvent);
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
        #if defined(NONBLOCKING)
            Wire.sendTransmission();          // Transmit to Slave, non-blocking
        #else
            Wire.endTransmission();           // Transmit to Slave, blocking
        #endif

        // After send complete, callback will print result

        digitalWrite(LED_BUILTIN,LOW);    // LED off
        delay(100);                       // 100ms delay
    }

    // Read string from Slave
    //
    if(digitalRead(11) == LOW)
    {
        digitalWrite(LED_BUILTIN,HIGH);   // LED on

        // Print message
        Serial.print("Reading from Slave: ");

        // Read from Slave
        #if defined(NONBLOCKING)
            Wire.sendRequest(target, (size_t)MEM_LEN); // Read from Slave (string len unknown, request full buffer), non-blocking
        #else
            Wire.requestFrom(target, (size_t)MEM_LEN); // Read from Slave (string len unknown, request full buffer), blocking
        #endif

        // After request complete, callback will print result

        digitalWrite(LED_BUILTIN,LOW);    // LED off
        delay(100);                       // 100ms delay
    }

    // Diagnostics - print error count summary
    //
    if(digitalRead(10) == LOW)
    {
        digitalWrite(LED_BUILTIN,HIGH);   // LED on

        // Print errors
        Serial.print("\n");
        Serial.printf("I2C_ERRCNT_RESET_BUS: %d\n", Wire.getErrorCount(I2C_ERRCNT_RESET_BUS));
        Serial.printf("I2C_ERRCNT_TIMEOUT:   %d\n", Wire.getErrorCount(I2C_ERRCNT_TIMEOUT));
        Serial.printf("I2C_ERRCNT_ADDR_NAK:  %d\n", Wire.getErrorCount(I2C_ERRCNT_ADDR_NAK));
        Serial.printf("I2C_ERRCNT_DATA_NAK:  %d\n", Wire.getErrorCount(I2C_ERRCNT_DATA_NAK));
        Serial.printf("I2C_ERRCNT_ARBL:      %d\n", Wire.getErrorCount(I2C_ERRCNT_ARBL));
        Serial.printf("I2C_ERRCNT_NOT_ACQ:   %d\n", Wire.getErrorCount(I2C_ERRCNT_NOT_ACQ));
        Serial.printf("I2C_ERRCNT_DMA_ERR:   %d\n", Wire.getErrorCount(I2C_ERRCNT_DMA_ERR));

        // uncomment to zero all errors after dumping
        //for(uint8_t i=0; i < I2C_ERRCNT_DMA_ERR; i++) Wire.zeroErrorCount((i2c_err_count)i);

        digitalWrite(LED_BUILTIN,LOW);    // LED off
        delay(100);                       // 100ms delay
    }
}

//
// Trigger after Tx complete (outgoing I2C data)
//
void transmitDone(void)
{
    Serial.print("OK\n");
}

//
// Trigger after Rx complete (incoming I2C data)
//
void requestDone(void)
{
    Wire.read(databuf, Wire.available());
    Serial.printf("'%s' OK\n",databuf);
}

//
// Trigger on I2C Error
//
void errorEvent(void)
{
    Serial.print("FAIL - ");
    switch(Wire.status())
    {
    case I2C_TIMEOUT:  Serial.print("I2C timeout\n"); Wire.resetBus(); break;
    case I2C_ADDR_NAK: Serial.print("Slave addr not acknowledged\n"); break;
    case I2C_DATA_NAK: Serial.print("Slave data not acknowledged\n"); break;
    case I2C_ARB_LOST: Serial.print("Arbitration Lost, possible pullup problem\n"); Wire.resetBus(); break;
    case I2C_BUF_OVF:  Serial.print("I2C buffer overflow\n"); break;
    case I2C_NOT_ACQ:  Serial.print("Cannot acquire bus, possible stuck SDA/SCL\n"); Wire.resetBus(); break;
    case I2C_DMA_ERR:  Serial.print("DMA Error\n"); break;
    default:           break;
    }
}
