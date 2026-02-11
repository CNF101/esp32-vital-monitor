# ESP32 Vital Monitor

Real time heart rate and blood oxygen monitor built using an Arduino Nano ESP32, MAX30102 optical sensor, and SSD1309 OLED display.

This project reads pulse signals from a fingertip, detects heart beats, computes beats per minute, estimates oxygen saturation, and presents results on a live screen.

Designed as a foundation for future wearable and connected health systems.

⸻

Features
	•	Real time BPM detection
	•	SpO₂ estimation
	•	Finger presence detection
	•	Signal filtering and smoothing
	•	I2C sensor communication
	•	OLED user interface
	•	Modular firmware design

⸻

Hardware Used
	•	Arduino Nano ESP32
	•	MAX30102 pulse oximeter
	•	SSD1309 128x64 I2C OLED
	•	Breadboard
	•	Jumper wires
	•	USB power

⸻

Wiring

Nano ESP32	MAX30102	OLED
3V3	VDD	VDD
GND	GND	GND
A4	SDA	SDA
A5	SCL	SCL


⸻

System Diagram


⸻

Real Build


⸻

How It Works

The MAX30102 emits red and infrared light into the finger.
Reflected intensity changes with blood volume.

The firmware:
	1.	Detects peaks in the IR waveform
	2.	Measures time between beats
	3.	Converts interval into BPM
	4.	Computes red to infrared ratio
	5.	Estimates oxygen saturation

Results update on the OLED in real time.

⸻

Running the Project
	1.	Connect hardware according to wiring table
	2.	Open the Arduino sketch
	3.	Select Arduino Nano ESP32
	4.	Upload
	5.	Place finger on sensor

⸻

Future Improvements
	•	Bluetooth streaming
	•	Mobile app integration
	•	Data logging
	•	Battery operation
	•	Custom enclosure

⸻

License

MIT
