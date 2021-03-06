# Autowatering - Arduino DIY project

The project based on Alex Gyvyer' project (https://github.com/AlexGyver/Auto-Pumps) for plants auto watering. All the code was modified. For programming I use VS Code + Platform IO (https://platformio.org/).

Potentially many pumps could be added but I assembled the circuit and designed the box (https://www.thingiverse.com/thing:3607432) for 2 pumps. But this box was too small to put there all cables and I used another one from AliExpress. Also I've added 2 buttons for force starting of the pumps.

* [Link to video on YouTube](#video-link)
* [The main improvements](#The-main-improvements)
* [What you need to repeat](#what-you-need-to-repeats)
* [How to connect](#how-to-connect)
* [Versions](#versions)
* [What next](#what-next)

![alt text][device]

<a id="video-link"></a>
## Link to video on YouTube
https://www.youtube.com/watch?v=Nk5SpDqHN3k

<a id="The-main-improvements"></a>
## The main improvements 
* Object-oriented and readable code, as much as possible
* Separated file for settings (pins, timeout time etc)
* Separated menu in the system interface to set working and waiting times of pumps
* Saving data in Arduino memory (EEPROM) happens when an user leaves menu settings and if changes are
* Minimum time for a pump to wait is 1 minute, to work - 1 second 
* Maximum time for a pump to wait is almost 10 days (-1 minute), to work - almost 1 day (-1 second) 
* Force start/stop pumps by buttons commands (double click, hold press/release)
* Different timers display remaining time to change a pump state, force start timer in seconds 
* Save timestamp (in seconds) of last pumps watering (added in version 1.1)
* Use HC-SR04 Ultrasonic Sensor to determine the fullness of the water tank  **(added in version 1.2)**

<a id="what-you-need-to-repeats"></a>
## What you need to repeat
![alt text][whatYouNeed]
* Arduino Nano
* 2 channel relay
* Lcd1602 module with I2C
* Rotary encoder
* Buttons x2
* Pumps (12V) x2 + lm2577 step up converter **(or use 5V without converter)**
* Power supply (5V)
* Wires to connect, circuit board
* Box
* Diodes 1N4007 x2 (optional - depends on type of your relay)
* Сapacitor 470uf or more (optional)
* Check valves x2 (optional)
* Water hose (optional)
* DS3231 (added in version 1.1)
* HC-SR04 Ultrasonic Sensor **(added in version 1.2)**

<a id="how-to-connect"></a>
## How to connect
See scheme of original project (https://github.com/AlexGyver/Auto-Pumps). I've done the same but I have 2 channel relay without diodes inside and I had to add them to the pumps outs. It looks like this:
![alt text][howToConnectDiodes]

Also I added capacitor in the scheme and connected it to arduino outs:
![alt text][howToConnectCapacitor]

The RTC module and Lcd screen must be connected via the I2C interface (A4 and A5 pins - SDA and SCL outputs). See the following image (from the Internet): 
![alt text][i2c_devices]

And I have different pins to connect modules you can find them in "settings.h" file:
```C++
#define I2C_LcdAddress 0x27
// #define DS3231_ADDRESS	      0x68 //I2C Slave address - from Sodaq_DS3231.cpp

#define PIN_EncoderClk 2
#define PIN_EncoderDt 3
#define PIN_EncoderSw 4

#define PIN_Button1 5
#define PIN_Button2 6

#define PIN_FirstPump 7
#define PUMP_AMOUNT 2

#define PIN_UltrasonicSensorTrigger A2
#define PIN_UltrasonicSensorEcho 12
```
I use 2 pumps (D7 and D8 outs in Arduino) but you can add more. See code how to implement that. Also, you can remove buttons if you don't need them and use their pins to connect extra pumps. 
As a water tank, I use a 10-liter plastic bottle and the height from ultrasonic sensor to the bottom of the bottle is about 30 cm - change the parameter in settings.h file for your tanks:
```C++
#define DISTANCE_TO_BOTTOM_CM 30
```
Possible, I will move a setup of the parameters to the program of the device in future.
<a id="versions"></a>
## Versions
[version 1.0](https://github.com/ppetrov-dev/Autowatering/tree/version-1.0)<br/>
[version 1.1](https://github.com/ppetrov-dev/Autowatering/tree/version-1.1)<br/>
version 1.2 - current

<a id="what-next"></a>
## What next
I don't have any certain plans for the project. I'd add a soil moisture sensor. Also, I want the device can work with several modes: by a timer, by a real-time clock and by moisture sensor or combinations of the modes. To set up modes and additional settings I will use a bigger display. Most likely it will be connected with the Internet. I'll create an account on Twitter for my plants and they will post how comfortable they live. And most probably when I finish all above I'd develop phone application for the device. Will see.

[whatYouNeed]: https://github.com/ppetrov-dev/Autowatering/raw/version-1.2/Screenshots/WhatYouNeed.png
[howToConnectDiodes]: https://github.com/ppetrov-dev/Autowatering/raw/version-1.2/Screenshots/HowToConnectDiodes.jpg
[howToConnectCapacitor]: https://github.com/ppetrov-dev/Autowatering/raw/version-1.2/Screenshots/HowToConnectCapacitor.png
[device]: https://github.com/ppetrov-dev/Autowatering/raw/version-1.2/Screenshots/device.png
[howToConnectCapacitor]: https://github.com/ppetrov-dev/Autowatering/raw/version-1.1/Screenshots/HowToConnectCapacitor.png
[i2c_devices]: https://github.com/ppetrov-dev/Autowatering/raw/version-1.1/Screenshots/i2c_devices.jpg
