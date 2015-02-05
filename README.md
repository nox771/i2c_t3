# i2c_t3
Enhanced I2C library for Teensy 3.x devices

This is an enhanced I2C library for [Teensy 3.x devices](http://pjrc.com/teensy/index.html).

Recent discussion and a usage summary can be found in the [PJRC forums here](https://forum.pjrc.com/threads/21680-New-I2C-library-for-Teensy3)

## **Description**

To use the library, unpack the the zip contents into your sketchbook/libraries folder. Zip file links are located at the end of this post.

To use with existing Arduino sketches, simply change the **#include <Wire.h>** to **#include <i2c_t3.h>**

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

