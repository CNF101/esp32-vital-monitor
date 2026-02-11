# Vital Monitor

A fingertip heart rate and blood oxygen monitor built with an Arduino
Nano ESP32, MAX30102 optical sensor, and SSD1309 OLED display.

The system samples infrared and red light, detects pulse peaks,
calculates beats per minute, estimates oxygen saturation, and presents
the results live on screen.

------------------------------------------------------------------------

## What the device does

-   Detects if a finger is present
-   Measures heart beats from blood volume changes
-   Calculates BPM from beat intervals
-   Estimates SpO₂ using red to infrared ratios
-   Displays information on an OLED
-   Continuously updates while the finger remains in place

------------------------------------------------------------------------

## Hardware

-   Arduino Nano ESP32
-   MAX30102 pulse oximeter sensor
-   SSD1309 I2C OLED display
-   Breadboard
-   Jumper wires
-   USB-C cable
-   Power Bank

------------------------------------------------------------------------

## Wiring

  All devices communicate using I2C.

 
| Nano ESP32 | MAX30102 | OLED |
|------------|----------|------|
| 3V3 | VDD | VDD |
| GND | GND | GND |
| A4 | SDA | SDA |
| A5 | SCL | SCL |


------------------------------------------------------------------------

## Libraries Required

Install from the Arduino Library Manager.

-   U8g2
-   SparkFun MAX3010x
-   heartRate

------------------------------------------------------------------------

## How to run
0.  Download Arduino IDE
1.  Wire the hardware according to the table
2.  Connect USB-C
3.  Open the `.ino` file
4.  Select **Arduino Nano ESP32** as the board and select the port you use
5.  Upload
6.  Open Serial Monitor if debugging
7.  Place your finger on the sensor

------------------------------------------------------------------------

## Display behavior

**No finger**\
The screen prompts the user to place a finger.

**Finger detected**\
BPM begins calculating.\
SpO₂ stabilizes after a short averaging window.\
Values refresh continuously.

------------------------------------------------------------------------

## Measurement method

Heart rate is derived from peak detection on the infrared waveform.

SpO₂ is estimated by comparing AC and DC components of red and infrared
light absorption.

Filtering reduces noise and motion artifacts.

------------------------------------------------------------------------

## Limitations

This project is a prototype for learning and experimentation.
**It is not a certified medical device.**

------------------------------------------------------------------------

## Future possible improvements

-   Bluetooth or WiFi data streaming capable due to ESP32
-   Mobile or web dashboard
-   Historical logging
-   Battery operation
-   Custom enclosure

------------------------------------------------------------------------

## License

MIT
