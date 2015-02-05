# i2c_t3
Enhanced I2C library for Teensy 3.x devices

This is an enhanced I2C library for [Teensy 3.x devices](http://pjrc.com/teensy/index.html).

Recent discussion and a usage summary can be found in the [PJRC forums here](https://forum.pjrc.com/threads/21680-New-I2C-library-for-Teensy3).

## **Description**

To use the library, unpack the the zip contents into your sketchbook/libraries folder. Zip file links are located at the end of this post.

To use with existing Arduino sketches, simply change the **#include \<Wire.h\>** to **#include \<i2c_t3.h\>**

Example sketches can be found in the Arduino menus at: **File->Examples->i2c_t3**

The latest version of the library provides the following:

* For **Teensy 3.0**, the I2C interface is: **Wire**
* For **Teensy 3.1**, the two I2C interfaces are: **Wire** and **Wire1**

Each interface has two sets of pins that it can utilize. Only one set of pins can be used at a time, but in a Master configuration the pins can be changed when the bus is idle.

* The **Wire** bus will communicate on pins **18(SDA0)/19(SCL0)** and **16(SCL0)/17(SDA0)**.
* The **Wire1** bus will communicate on pins **29(SCL1)/30(SDA1)** and **26(SCL1)/31(SDA1)**.

Unfortunately the Wire1 connections are all on the surface mount backside pads. It is recommended to use a breakout expansion board to access those, as the pads are likely not mechanically "robust", with respect to soldered wires pulling on them.

As far as voltage levels, the Teensy 3.0 I2C pins are 3.3V tolerant, and the Teensy 3.1 I2C pins are 5V tolerant. To connect 5V devices to Teensy 3.0 or to connect multiple voltage level I2C buses, refer to the following app note by NXP:
http://www.nxp.com/documents/application_note/AN10441.pdf

The following sections outline the included examples, modifiable header defines, and function summary. Most all functions are demonstrated in the example files.

## **Clocking**

The library now supports all Teensyduino **F_BUS** frequencies: **60MHz, 56MHz, 48MHz, 36MHz, 24MHz, 16MHz, 8MHz, 4MHz, 2MHz**.

The supported rates depend on the F_BUS setting which in turn depends on the F_CPU setting. The current F_CPU -> F_BUS mapping (Teensyduino 1.21), is as follows. For a given F_BUS, if an unsupported rate is given, then the highest supported rate available is used (since unsupported rates fall off from the high end).

```
                                             I2C_RATE (kHz)
F_CPU    F_BUS    3000 2800 2400 2000 1800 1500 1200 1000  800  600  400  300  200  100
-----    -----    ---------------------------------------------------------------------
 168M     56M            y    y    y    y    y    y    y    y    y    y    y    y    y
 144M     48M                 y    y    y    y    y    y    y    y    y    y    y    y
 120M     60M       y    y    y    y    y    y    y    y    y    y    y    y    y    y
  96M     48M                 y    y    y    y    y    y    y    y    y    y    y    y
  72M     36M                           y    y    y    y    y    y    y    y    y    y
  48M     48M                 y    y    y    y    y    y    y    y    y    y    y    y
  24M     24M                                     y    y    y    y    y    y    y    y
  16M     16M                                               y    y    y    y    y    y
   8M      8M                                                         y    y    y    y
   4M      4M                                                                   y    y
   2M      2M                                                                        y
```

The rates are not directly equivalent to SCL clock speeds. The peripheral limits the actual SCL speeds to well below the theoretical speeds. To get a better idea of throughput I've measured the transfer time for a 128 byte transfer across different F_CPU / F_BUS / I2C_RATE combinations (specifically the interesting overclock speeds). This is shown below.

![I2C Speed Test](speedtest.jpg)

## **Operational Modes**

There are three modes of operation: **Interrupt**, **DMA**, and **Immediate**. The operating mode of the I2C can be set in the **begin()** or **setOpMode()** functions, using the opMode parameter which can have the following values:

* I2C_OP_MODE_ISR - Interrupt
* I2C_OP_MODE_DMA - DMA
* I2C_OP_MODE_IMM - Immediate 

**Interrupt** mode is the normal default mode (it was the only mode in library versions prior to v7). It supports both Master and Slave operation. The two other modes, **DMA** and **Immediate**, are for Master operation only.

DMA mode requires an available DMA channel to operate. In cases where DMA mode is specified, but there are no available channels, then the I2C will revert to operating in Interrupt mode.

Similarly, for Interrupt mode to work the I2C ISRs must run at a higher priority than the calling function. Where this is not the case, the library will first attempt to elevate the priority of the I2C ISR to a higher priority than the calling function. If that is not possible then it will revert to operating in Immediate mode.

## **Example List**

### **Teensy 3.0/3.1 Examples**

**master** - this creates a Master device which is setup to talk to the Slave device given in the slave sketch.

**multiplexed_master** - this creates a Master device which can output I2C commands on either pins 18/19 or pins 16/17, and change pins on-the-fly. It can therefore operate as a Master on two I2C buses (in multiplexed mode, not simultaneous).

**scanner** - this creates a Master device which will scan the address space and report all devices which ACK.

**slave** - this creates a Slave device with simple read/write commands and a small addressable memory.

**slave_range** - this creates a Slave device which will respond to a range of I2C addresses. A function exists to obtain the Rx address, therefore it can be used to make devices which act as multiple I2C Slaves.

**timeout** - this creates a Master device which is setup to talk to the Slave device given in the slave sketch. It illustrates the use of different timeout functions.

**interrupt** - this creates a Master device which is setup to periodically read from a Slave device using a timer interrupt.

### **Teensy 3.1 ONLY Examples:**

**dual_bus_master_slave** - this creates a device using one bus as a Master and one bus as a Slave. This is particularly useful for Master/Slave development as they can be wired together, creating a closed test environment in a single device.

**quad_master** - utilizing both I2C interfaces and both sets of pins for each interface, this creates a device which can operate as a Master on four independent buses. The Wire bus will communicate on pins 18(SDA0)/19(SCL0) and 16(SCL0)/17(SDA0). The Wire1 bus will communicate on pins 29(SCL1)/30(SDA1) and 26(SCL1)/31(SDA1).

## **Header Defines**

These defines can be modified at the top of the **i2c_t3.h** file.

* **I2C_BUS_ENABLE n** - this is a Teensy 3.1 only define, which controls how many buses are enabled. When set as "I2C_BUS_ENABLE 1" only Wire will be active and code/ram size will be equivalent to Teensy 3.0. When set as "I2C_BUS_ENABLE 2" then both Wire and Wire1 will be active and code/ram usage will be increased.

* **I2C_TX_BUFFER_LENGTH n**
* **I2C_RX_BUFFER_LENGTH n** - these two defines control the buffers allocated to transmit/receive functions. When dealing with Slaves which don't need large communication (eg. sensors or such), these buffers can be reduced to a smaller size. Buffers should be large enough to hold: Target Addr + Target Command (varies with protocol) + Data payload. Default is: 259 bytes = 1 byte Addr + 2 byte Command + 256 byte Data.

* **I2C0_INTR_FLAG_PIN p**
* **I2C1_INTR_FLAG_PIN p** - these defines make the specified pin high whenever the I2C interrupt occurs (I2C0 == Wire, and I2C1 == Wire1). This is useful as a trigger signal when using a logic analyzer. By default they are undefined (commented out).

## **Function Summary**

The functions are divided into two classifications:

* _**Italic**_ functions are compatible with the original Arduino Wire API. This allows existing Arduino sketches to compile without modification.
* **Bold** functions are the added enhanced functions. They utilize the advanced capabilities of the Teensy 3.0/3.1 hardware. The library provides the greatest benefit when utilizing these functions (versus the standard Wire library). 


_**Wire.begin();**_ - initializes I2C as Master mode, pins 18/19 (Wire) or pins 29/30 (Wire1), external pullups, 100kHz rate

* return: none


_**Wire.begin(address);**_ - initializes I2C as Slave mode using address, pins 18/19 (Wire) or pins 29/30 (Wire1), external pullups, 100kHz rate

* return: none
* parameters:
    * address = 7bit slave address of device 

	
**Wire.begin(mode, address, pins, pullup, rate);** - initializes I2C as Master or single address Slave

* return: none
* parameters:
    * mode = I2C_MASTER, I2C_SLAVE
    * address = 7bit slave address when configured as Slave (ignored for Master mode)
    * pins = (Wire) I2C_PINS_18_19, I2C_PINS_16_17 or (Wire1) I2C_PINS_29_30, I2C_PINS_26_31
    * pullup = I2C_PULLUP_EXT, I2C_PULLUP_INT
    * rate = I2C_RATE_100, I2C_RATE_200, I2C_RATE_300, I2C_RATE_400, I2C_RATE_600, I2C_RATE_800, I2C_RATE_1000, I2C_RATE_1200, I2C_RATE_1500, I2C_RATE_1800, I2C_RATE_2000, I2C_RATE_2400, I2C_RATE_2800, I2C_RATE_3000 

	
**Wire.begin(mode, address, pins, pullup, rate, opMode);** - initializes I2C as Master or single address Slave

* return: none
* parameters:
    * mode = I2C_MASTER, I2C_SLAVE
    * address = 7bit slave address when configured as Slave (ignored for Master mode)
    * pins = (Wire) I2C_PINS_18_19, I2C_PINS_16_17 or (Wire1) I2C_PINS_29_30, I2C_PINS_26_31
    * pullup = I2C_PULLUP_EXT, I2C_PULLUP_INT
    * rate = I2C_RATE_100, I2C_RATE_200, I2C_RATE_300, I2C_RATE_400, I2C_RATE_600, I2C_RATE_800, I2C_RATE_1000, I2C_RATE_1200, I2C_RATE_1500, I2C_RATE_1800, I2C_RATE_2000, I2C_RATE_2400, I2C_RATE_2800, I2C_RATE_3000
    * opMode = I2C_OP_MODE_ISR, I2C_OP_MODE_DMA, I2C_OP_MODE_IMM 


**Wire.begin(mode, address1, address2, pins, pullup, rate);** - initializes I2C as Master or address range Slave

* return: none
* parameters:
    * mode = I2C_MASTER, I2C_SLAVE
    * address1 = 1st 7bit address for specifying Slave address range (ignored for Master mode)
    * address2 = 2nd 7bit address for specifying Slave address range (ignored for Master mode)
    * pins = (Wire) I2C_PINS_18_19, I2C_PINS_16_17 or (Wire1) I2C_PINS_29_30, I2C_PINS_26_31
    * pullup = I2C_PULLUP_EXT, I2C_PULLUP_INT
    * rate = I2C_RATE_100, I2C_RATE_200, I2C_RATE_300, I2C_RATE_400, I2C_RATE_600, I2C_RATE_800, I2C_RATE_1000, I2C_RATE_1200, I2C_RATE_1500, I2C_RATE_1800, I2C_RATE_2000, I2C_RATE_2400, I2C_RATE_2800, I2C_RATE_3000 


**Wire.begin(mode, address1, address2, pins, pullup, rate, opMode);** - initializes I2C as Master or address range Slave

* return: none
* parameters:
    * mode = I2C_MASTER, I2C_SLAVE
    * address1 = 1st 7bit address for specifying Slave address range (ignored for Master mode)
    * address2 = 2nd 7bit address for specifying Slave address range (ignored for Master mode)
    * pins = (Wire) I2C_PINS_18_19, I2C_PINS_16_17 or (Wire1) I2C_PINS_29_30, I2C_PINS_26_31
    * pullup = I2C_PULLUP_EXT, I2C_PULLUP_INT
    * rate = I2C_RATE_100, I2C_RATE_200, I2C_RATE_300, I2C_RATE_400, I2C_RATE_600, I2C_RATE_800, I2C_RATE_1000, I2C_RATE_1200, I2C_RATE_1500, I2C_RATE_1800, I2C_RATE_2000, I2C_RATE_2400, I2C_RATE_2800, I2C_RATE_3000
    * opMode = I2C_OP_MODE_ISR, I2C_OP_MODE_DMA, I2C_OP_MODE_IMM 


**Wire.setOpMode(opMode);** - this configures operating mode of the I2C as either Immediate, ISR, or DMA. By default Arduino-style begin() calls will initialize to ISR mode. This can only be called when the bus is idle (no changing mode in the middle of Tx/Rx). Note that Slave mode can only use ISR operation.

* return: 1=success, 0=fail (bus busy)
* parameters:
    * opMode = I2C_OP_MODE_ISR, I2C_OP_MODE_DMA, I2C_OP_MODE_IMM 


**Wire.setRate(busFreq, rate);** - reconfigures I2C frequency divider based on supplied bus freq and desired rate (rate in this case is an enum). If an unsupported busFreq / rate combination is specified, then the highest available rate will be used, and it will flag an error. Bus frequency is supplied here to allow for applications that alter their running frequency to recalibrate the I2C to a known rate given their frequency. Bus frequency must still be one of the supported frequencies.

* return: 1=success, 0=fail (incompatible bus freq & I2C rate combination)
* parameters:
    * busFreq = bus frequency, typically F_BUS unless reconfigured
    * rate = I2C_RATE_100, I2C_RATE_200, I2C_RATE_300, I2C_RATE_400, I2C_RATE_600, I2C_RATE_800, I2C_RATE_1000, I2C_RATE_1200, I2C_RATE_1500, I2C_RATE_1800, I2C_RATE_2000, I2C_RATE_2400, I2C_RATE_2800, I2C_RATE_3000 


**Wire.setRate(busFreq, i2cFreq);** - reconfigures I2C frequency divider based on supplied bus freq and desired I2C freq. I2C frequency in this case is quantized to an approximate I2C_RATE based on actual SCL frequency measurements using 48MHz bus as a basis. This is done for simplicity, as theoretical SCL freq do not correlate to actual freq very well.

* return: 1=success, 0=fail (incompatible bus freq & I2C rate combination)
* parameters:
    * busFreq = bus frequency, typically F_BUS unless reconfigured
    * i2cFreq = desired I2C frequency (will be quantized to nearest rate) 


_**Wire.setClock(i2cFreq);**_ - reconfigures I2C frequency divider to get desired I2C freq. Bus frequency is fixed at F_BUS. I2C frequency in this case is quantized to an approximate I2C_RATE based on actual SCL frequency measurements using 48MHz bus as a basis. This is done for simplicity, as theoretical SCL freq do not correlate to actual freq very well.

* return: 1=success, 0=fail (incompatible bus freq & I2C rate combination)
* parameters:
    * i2cFreq = desired I2C frequency (will be quantized to nearest rate) 


**Wire.pinConfigure(pins, pullup);** - reconfigures active I2C pins on-the-fly (only works when bus is idle). Inactive pins will switch to input mode.

* return: 1=success, 0=fail
* parameters:
    * pins = (Wire) I2C_PINS_18_19, I2C_PINS_16_17 or (Wire1) I2C_PINS_29_30, I2C_PINS_26_31
    * pullup = I2C_PULLUP_EXT, I2C_PULLUP_INT 


_**Wire.beginTransmission(address);**_ - initialize Tx buffer for transmit to slave at address

* return: none
* parameters:
    * address = target 7bit slave address 


_**Wire.endTransmission();**_ - blocking routine, transmits Tx buffer to slave

* return: 0=success, 1=data too long, 2=recv addr NACK, 3=recv data NACK, 4=other error


**Wire.endTransmission(i2c_stop);** - blocking routine, transmits Tx buffer to slave. i2c_stop parameter can be used to indicate if command should end with a STOP (I2C_STOP) or not (I2C_NOSTOP).

* return: 0=success, 1=data too long, 2=recv addr NACK, 3=recv data NACK, 4=other error
* parameters:
    * i2c_stop = I2C_NOSTOP, I2C_STOP 


**Wire.endTransmission(i2c_stop, timeout);** - blocking routine with timeout, transmits Tx buffer to slave. i2c_stop parameter can be used to indicate if command should end with a STOP(I2C_STOP) or not (I2C_NOSTOP).

* return: 0=success, 1=data too long, 2=recv addr NACK, 3=recv data NACK, 4=other error
* parameters:
    * i2c_stop = I2C_NOSTOP, I2C_STOP
    * timeout = timeout in microseconds 


**Wire.sendTransmission();** - non-blocking routine, starts transmit of Tx buffer to slave (implicit I2C_STOP). Use done() or finish() to determine completion and status() to determine success/fail.

* return: none


**Wire.sendTransmission(i2c_stop);** - non-blocking routine, starts transmit of Tx buffer to slave. i2c_stop parameter can be used to indicate if command should end with a STOP (I2C_STOP) or not (I2C_NOSTOP). Use done() or finish() to determine completion and status() to determine success/fail.

* return: none
* parameters:
    * i2c_stop = I2C_NOSTOP, I2C_STOP 


_**Wire.requestFrom(address, length);**_ - blocking routine, requests length bytes from slave at address. Receive data will be placed in the Rx buffer.

* return: #bytes received = success, 0=fail
* parameters:
    * address = target 7bit slave address
    * length = number of bytes requested 


**Wire.requestFrom(address, length, i2c_stop);** - blocking routine, requests length bytes from slave at address. Receive data will be placed in the Rx buffer. i2c_stop parameter can be used to indicate if command should end with a STOP (I2C_STOP) or not (I2C_NOSTOP).

* return: #bytes received = success, 0=fail
* parameters:
    * address = target 7bit slave address
    * length = number of bytes requested
    * i2c_stop = I2C_NOSTOP, I2C_STOP 


**Wire.requestFrom(address, length, i2c_stop, timeout);** - blocking routine with timeout, requests length bytes from slave at address. Receive data will be placed in the Rx buffer. i2c_stop parameter can be used to indicate if command should end with a STOP (I2C_STOP) or not (I2C_NOSTOP).

* return: #bytes received = success, 0=fail
* parameters:
    * address = target 7bit slave address
    * length = number of bytes requested
    * i2c_stop = I2C_NOSTOP, I2C_STOP
    * timeout = timeout in microseconds 


**Wire.sendRequest(address, length, i2c_stop);** - non-blocking routine, starts request for length bytes from slave at address. Receive data will be placed in the Rx buffer. i2c_stop parameter can be used to indicate if command should end with a STOP (I2C_STOP) or not (I2C_NOSTOP). Use done() or finish() to determine completion and status() to determine success/fail.

* return: none
* parameters:
    * address = target 7bit slave address
    * length = number of bytes requested
    * i2c_stop = I2C_NOSTOP, I2C_STOP 


**Wire.getError();** - returns "Wire" error code from a failed Tx/Rx command

* return: 0=success, 1=data too long, 2=recv addr NACK, 3=recv data NACK, 4=other error


**Wire.status();** - returns current status of I2C (enum return value)

* return: I2C_WAITING, I2C_SENDING, I2C_SEND_ADDR, I2C_RECEIVING, I2C_TIMEOUT, I2C_ADDR_NAK, I2C_DATA_NAK, I2C_ARB_LOST, I2C_SLAVE_TX, I2C_SLAVE_RX


**Wire.done();** - returns simple complete/not-complete value to indicate I2C status

* return: 1=Tx/Rx complete (with or without errors), 0=still running


**Wire.finish();** - blocking routine, loops until Tx/Rx is complete

* return: 1=Tx/Rx complete (Tx or Rx completed, no error), 0=fail (NAK, timeout or Arb lost)


**Wire.finish(timeout);** - blocking routine with timeout, loops until Tx/Rx is complete or timeout occurs

* return: 1=Tx/Rx complete (Tx or Rx completed, no error), 0=fail (NAK, timeout or Arb lost)
* parameters:
    * timeout = timeout in microseconds 


_**Wire.write(data);**_ - write data byte to Tx buffer

* return: #bytes written = success, 0=fail
* parameters:
    * data = data byte 


_**Wire.write(data, length);**_ - write length number of bytes from data array to Tx buffer

* return: #bytes written = success, 0=fail
* parameters:
    * data = pointer to uint8_t (or char) array of data
    * length = number of bytes to write 


_**Wire.available();**_ - returns number of remaining available bytes in Rx buffer

* return: #bytes available


_**Wire.read();**_ - returns next data byte (signed int) from Rx buffer

* return: data, -1 if buffer empty


_**Wire.peek();**_ - returns next data byte (signed int) from Rx buffer without removing it from Rx buffer

* return: data, -1 if buffer empty


**Wire.readByte();** - returns next data byte (uint8_t) from Rx buffer

* return: data, 0 if buffer empty


**Wire.peekByte();** - returns next data byte (uint8_t) from Rx buffer without removing it from Rx buffer

* return: data, 0 if buffer empty

_**Wire.flush();**_ - does nothing


**Wire.getRxAddr();** - returns target address of incoming I2C command. Used for Slaves operating over an address range.

* return: rxAddr of last received command


_**Wire.onReceive(function);**_ - used to set Slave Rx callback, refer to code examples


_**Wire.onRequest(function);**_ - used to set Slave Tx callback, refer to code examples

	
## **Compatible Libraries**

These are libraries which are known to be compatible with this I2C library. They may have been possibly modified to utilize enhanced functions (higher speed, timeouts, etc), or perhaps for general compatibility. Please contact their respective authors for questions regarding their usage.

* Arduino sketch for MPU-9250 9DoF with AHRS sensor fusion - https://github.com/kriswiner/MPU-9250
* LSM9DS0 9DOF sensor AHRS sketch - https://github.com/kriswiner/LSM9DS0
* Adafruit FRAM board - https://bitbucket.org/JezWeston/adafruit_fram_i2c_t3 