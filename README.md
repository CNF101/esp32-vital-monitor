# Vital Monitor

A fingertip heart rate and blood oxygen monitor built with an Arduino Nano ESP32, MAX30105 optical sensor, and SSD1309 OLED display.

The system samples infrared and red light at 400Hz, detects pulse peaks in real-time, calculates beats per minute using a moving average, estimates oxygen saturation using the Maxim algorithm, and presents live results on a crisp OLED screen with animated indicators.

## What the device does

* **Finger detection** - Automatically detects when a finger is placed on the sensor
* **Real-time heart rate** - Measures BPM from blood volume changes with beat-to-beat accuracy
* **Averaged BPM calculation** - Uses a 4-beat rolling average for stable readings
* **SpO₂ estimation** - Calculates blood oxygen saturation using red/IR light ratios
* **Smart update rate** - BPM updates with every heartbeat; SpO₂ updates every 3 seconds for optimal accuracy
* **Live OLED display** - Shows measurements with animated heartbeat icon and progress indicators
* **Non-blocking operation** - Responsive UI that never freezes during calculations
* **Auto-reset** - Clears all measurements when finger is removed

## Hardware

* **Arduino Nano ESP32** - Main microcontroller
* **MAX30105 pulse oximeter sensor** - Optical heart rate and SpO₂ sensor (also compatible with MAX30102)
* **SSD1309 128×64 2.42" I2C OLED display** - High-contrast monochrome display
* **Breadboard** - For prototyping
* **Jumper wires** - Male-to-female recommended
* **USB-C cable** - For programming and power
* **Power bank** (optional) - For portable operation

## Wiring

All devices communicate using I2C.

| Nano ESP32 | MAX30105 | OLED |
|------------|----------|------|
| 3V3        | VDD      | VDD  |
| GND        | GND      | GND  |
| A4         | SDA      | SDA  |
| A5         | SCL      | SCL  |

**I2C Configuration:**
* SDA: A4 (GPIO 6)
* SCL: A5 (GPIO 7)
* Clock speed: 400kHz (Fast Mode)

## Libraries Required

Install these from the Arduino Library Manager:

* **U8g2** - OLED graphics library
* **SparkFun MAX3010x** - Sensor driver library
* **heartRate** - Beat detection algorithm (included with SparkFun MAX3010x)
* **spo2_algorithm** - Maxim SpO₂ calculation algorithm (included with SparkFun MAX3010x)

## How to run

1. Download and install [Arduino IDE](https://www.arduino.cc/en/software)
2. Install the required libraries via Library Manager
3. Wire the hardware according to the table above
4. Connect the Arduino Nano ESP32 via USB-C
5. Open the `.ino` file in Arduino IDE
6. Select **Arduino Nano ESP32** as the board (Tools → Board)
7. Select the correct port (Tools → Port)
8. Click **Upload**
9. Open **Serial Monitor** (Tools → Serial Monitor, 115200 baud) for debug output
10. Place your finger **firmly** on the sensor and wait 5-10 seconds for measurements to stabilize

## Display behavior

### Landing screen (no finger)
* Animated heart icon with pulsing wave
* "Place Finger" prompt

### Measurement screen (finger detected)
* **Left panel:** Real-time BPM with animated heartbeat icon
* **Right panel:** SpO₂ percentage
* **Bottom status bar:**
  * "Calibrating" - Initial 100-sample buffer filling with progress bar
  * "Measuring..." - Waiting for stable readings
  * Progress indicator - Shows measurement duration

### Update rates
* **BPM:** Updates immediately with each detected heartbeat
* **SpO₂:** Recalculates every 3 seconds for accuracy and UI stability

## Measurement method

### Heart rate detection
* Samples infrared LED signal at **400 Hz** with 4× hardware averaging (100 effective samples/sec)
* Uses **checkForBeat()** algorithm for real-time peak detection
* Calculates instantaneous BPM from beat-to-beat intervals
* Applies **4-beat moving average** filter for stable display
* Only accepts physiologically valid beats (20-255 BPM)

### SpO₂ calculation
* Collects **100-sample buffers** of red and infrared light data
* Uses **Maxim integrated algorithm** (`maxim_heart_rate_and_oxygen_saturation`)
* Analyzes AC/DC ratios of both wavelengths
* Processes 25 new samples every 3 seconds after initial calibration
* Validates results within physiological range (70-100%)

### Noise reduction
* Hardware averaging reduces high-frequency noise
* Beat validation filters out motion artifacts
* Moving average smooths BPM fluctuations
* Buffer-based processing for SpO₂ stability

## Code features

* **Non-blocking state machine** - No `delay()` calls, responsive UI
* **Watchdog feeding** - Prevents ESP32 resets during long operations
* **Memory-safe buffer management** - Fixed 100-sample circular buffers
* **Type-safe sensor readings** - Uses `uint32_t` to prevent ESP32 bit-level issues
* **Smart UI updates** - 100ms refresh rate without blocking sensor sampling
* **Automatic calibration** - Self-adjusts on finger placement

## Limitations

⚠️ **This project is a prototype for learning and experimentation. It is NOT a certified medical device.**

* Not FDA approved or medically validated
* Should not be used for clinical decisions
* Readings may vary based on finger placement, pressure, and temperature
* Motion and ambient light can affect accuracy
* Best used in a stationary position with good finger contact

## Troubleshooting

**"Sensor not found" error:**
* Check wiring connections
* Verify I2C addresses (use I2C scanner sketch)
* Ensure 3.3V power supply is stable

**BPM shows "---":**
* Press finger more firmly on sensor
* Ensure finger covers both LEDs completely
* Wait 5-10 seconds for stabilization
* Try different finger

**SpO₂ not updating:**
* Initial calibration takes ~1 second (100 samples)
* SpO₂ updates every 3 seconds by design
* Ensure stable finger placement

## Future possible improvements

* **Wireless connectivity** - Bluetooth or WiFi data streaming (ESP32 capable)
* **Mobile app** - Real-time monitoring on smartphone
* **Web dashboard** - Cloud-based data visualization
* **Historical logging** - Store and analyze trends over time
* **Battery operation** - LiPo battery with charging circuit
* **Custom enclosure** - 3D printed case for portability
* **Additional sensors** - Temperature, accelerometer for motion detection
* **Deep sleep mode** - Extended battery life when idle

## License

MIT
