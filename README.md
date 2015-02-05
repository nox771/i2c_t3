# i2c_t3
Enhanced I2C library for Teensy 3.x devices

This is an enhanced I2C library for [Teensy 3.x devices](http://pjrc.com/teensy/index.html).

Recent discussion and a usage summary can be found in the [PJRC forums here](https://forum.pjrc.com/threads/21680-New-I2C-library-for-Teensy3).

## **Description**

To use the library, unpack the the zip contents into your sketchbook/libraries folder. Zip file links are located at the end of this post.

To use with existing Arduino sketches, simply change the **#include \<Wire.h\>** to **#include \<i2c_t3.h\>**

Example sketches can be found in the Arduino menus at: **File->Examples->i2c_t3**

The latest version of the library provides the following:

For **Teensy 3.0**, the I2C interface is: **Wire**

For **Teensy 3.1**, the two I2C interfaces are: **Wire** and **Wire1**

Each interface has two sets of pins that it can utilize. Only one set of pins can be used at a time, but in a Master configuration the pins can be changed when the bus is idle.

The **Wire** bus will communicate on pins **18(SDA0)/19(SCL0)** and **16(SCL0)/17(SDA0)**.

The **Wire1** bus will communicate on pins **29(SCL1)/30(SDA1)** and **26(SCL1)/31(SDA1)**.

Unfortunately the Wire1 connections are all on the surface mount backside pads. It is recommended to use a breakout expansion board to access those, as the pads are likely not mechanically "robust", with respect to soldered wires pulling on them.

As far as voltage levels, the Teensy 3.0 I2C pins are 3.3V tolerant, and the Teensy 3.1 I2C pins are 5V tolerant. To connect 5V devices to Teensy 3.0 or to connect multiple voltage level I2C buses, refer to the following app note by NXP:
http://www.nxp.com/documents/application_note/AN10441.pdf

The following sections outline the included examples, modifiable header defines, and function summary. Most all functions are demonstrated in the example files.

## **Clocking**

The library now supports all Teensyduino **F_BUS** frequencies: **60MHz, 56MHz, 48MHz, 36MHz, 24MHz, 16MHz, 8MHz, 4MHz, 2MHz**

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
