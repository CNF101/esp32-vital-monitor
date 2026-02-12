#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include "spo2_algorithm.h"
#include <U8g2lib.h>

// ========================================
// HARDWARE
// ========================================
MAX30105 particleSensor;
U8G2_SSD1309_128X64_NONAME0_1_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// ========================================
// CONSTANTS
// ========================================
const int      DRAW_INTERVAL    = 100;
const uint32_t FINGER_THRESHOLD = 50000;
const unsigned long SPO2_UPDATE_INTERVAL = 3000; // Update SpO2 every 3 seconds

// ========================================
// HEARTBEAT Variables
// ========================================
const byte RATE_SIZE = 4;
byte  rates[RATE_SIZE];
byte  rateSpot        = 0;
long  lastBeat        = 0;
float beatsPerMinute  = 0;
int   beatAvg         = 0;
byte  validBeats      = 0;

// ========================================
// SPO2 Variables
// ========================================
uint32_t irBuffer[100];
uint32_t redBuffer[100];
int32_t  bufferLength   = 100;
int32_t  spo2           = 0;
int8_t   validSPO2      = 0;
int32_t  heartRate      = 0;  
int8_t   validHeartRate = 0;

// Non-blocking state machine replacing SparkFun's blocking for/while loops
enum SpO2State { FILLING, CALCULATING, SHIFTING, COLLECTING, WAITING };
SpO2State spo2State  = FILLING;
int       fillIndex  = 0;   // index during FILLING  (0..99)
int       collectIdx = 75;  // index during COLLECTING (75..99)
unsigned long lastSpo2Update = 0; // Track when we last updated SpO2

// ========================================
// UI STATE
// ========================================
unsigned long lastDraw         = 0;
byte          heartBeat        = 0;
byte          heartScale       = 0;
unsigned long animTimer        = 0;
bool          fingerPresent    = false;
unsigned long measureStartTime = 0;
unsigned long lastYield        = 0;

// ========================================
// FORWARD DECLARATIONS
// ========================================
void render(bool finger);
void showLanding();
void showMeasurements();
void showWaitingIndicator(int x, int y, int width);

// ========================================
// WATCHDOG
// ========================================
void feedWatchdog() {
  if (millis() - lastYield > 10) {
    lastYield = millis();
    yield();
  }
}

// ========================================
// RESET
// ========================================
void resetAll() {
  beatsPerMinute = 0;
  beatAvg        = 0;
  rateSpot       = 0;
  lastBeat       = 0;
  validBeats     = 0;
  memset(rates, 0, sizeof(rates));

  spo2           = 0;
  validSPO2      = 0;
  heartRate      = 0;
  validHeartRate = 0;

  spo2State  = FILLING;
  fillIndex  = 0;
  collectIdx = 75;
  lastSpo2Update = 0;
}

// ========================================
// SETUP
// ========================================
void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("Initializing...");

  Wire.begin(A4, A5);
  Wire.setClock(400000);

  u8g2.begin();
  u8g2.setPowerSave(0);
  u8g2.setContrast(255);

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30105 was not found. Please check wiring/power.");
    u8g2.firstPage();
    do {
      u8g2.setFont(u8g2_font_6x10_tr);
      u8g2.drawStr(10, 30, "Sensor not found!");
      u8g2.drawStr(15, 45, "Check wiring.");
    } while (u8g2.nextPage());
    while (1) { delay(100); yield(); }
  }

  // sampleRate=400 + sampleAverage=4 = 100 effective samples/sec
  // CRITICAL for checkForBeat() — at sampleRate=100 you only get 25sps
  // which is ~1 sample per heartbeat peak and detection always fails.
  // At 400sps / avg4 = 100sps you get ~5 samples per peak — works reliably.
  // SpO2 still works: Maxim algorithm just needs 100 consistent samples.
  particleSensor.setup(
    60,    // ledBrightness
    4,     // sampleAverage
    2,     // ledMode: Red + IR
    400,   // sampleRate — was 100, MUST be 400 for checkForBeat to work
    411,   // pulseWidth
    4096   // adcRange
  );

  particleSensor.clearFIFO();
  Serial.println("Sensor ready. Place finger firmly on sensor.");
}

// ========================================
// MAIN LOOP
// ========================================
void loop() {
  feedWatchdog();

  // ---- Display refresh ----
  if (millis() - lastDraw >= DRAW_INTERVAL) {
    lastDraw = millis();
    render(fingerPresent);
  }

  // Heartbeat animation decay
  if (heartBeat > 12) heartBeat -= 12; else heartBeat = 0;

  // Landing page animation
  if (millis() - animTimer > 60) {
    animTimer = millis();
    if (++heartScale > 20) heartScale = 0;
  }

  // ---- WAITING: delay between SpO2 calculations ----
  if (spo2State == WAITING) {
    if (millis() - lastSpo2Update >= SPO2_UPDATE_INTERVAL) {
      spo2State = COLLECTING;
      collectIdx = 75;
      Serial.println("[SpO2] Wait complete -> COLLECTING");
    }
    // Continue reading samples for heart rate, just skip SpO2 processing
  }

  // ---- CALCULATING: blocking — force display refresh first ----
  if (spo2State == CALCULATING) {
    // Don't render here - let it happen naturally in the display refresh cycle
    maxim_heart_rate_and_oxygen_saturation(
      irBuffer, bufferLength, redBuffer,
      &spo2, &validSPO2,
      &heartRate, &validHeartRate
    );
    Serial.print("[SpO2] spo2="); Serial.print(spo2);
    Serial.print(" valid="); Serial.println(validSPO2);
    lastSpo2Update = millis();
    spo2State = SHIFTING;
    return;
  }

  // ---- SHIFTING: fast, no sample needed ----
  if (spo2State == SHIFTING) {
    for (byte i = 25; i < 100; i++) {
      redBuffer[i - 25] = redBuffer[i];
      irBuffer[i - 25]  = irBuffer[i];
    }
    spo2State = WAITING; // Go to WAITING instead of directly to COLLECTING
    return;
  }

  // ========================================================
  // READ SENSOR SAMPLE
  // Use check()/available()/nextSample() for the SpO2 buffer.
  // Also call getIR() for checkForBeat() using uint32_t —
  // CRITICAL on ESP32: must be uint32_t, not long, because
  // checkForBeat() does bit-level math that breaks with signed types.
  // ========================================================
  particleSensor.check();
  if (!particleSensor.available()) return;

  uint32_t irValue  = particleSensor.getIR();   // uint32_t — NOT long
  uint32_t redValue = particleSensor.getRed();  // uint32_t — NOT long
  particleSensor.nextSample();

  // ---- Finger detection ----
  if (irValue < FINGER_THRESHOLD) {
    if (fingerPresent) {
      Serial.println("No finger — resetting");
      fingerPresent = false;
      resetAll();
    }
    return;
  }

  if (!fingerPresent) {
    Serial.println("Finger detected");
    fingerPresent    = true;
    measureStartTime = millis();
  }

  // ========================================================
  // checkForBeat() receives uint32_t cast to long as the
  // library expects — values fit safely in 32-bit signed
  // because MAX30102 IR tops out around 262143 (18-bit).
  // ========================================================
  if (checkForBeat((long)irValue) == true) {
    long delta     = millis() - lastBeat;
    lastBeat       = millis();
    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute < 255 && beatsPerMinute > 20) {
      rates[rateSpot++] = (byte)beatsPerMinute;
      rateSpot %= RATE_SIZE;

      // Only average real slots, not zero-initialized ones
      if (validBeats < RATE_SIZE) validBeats++;
      beatAvg = 0;
      for (byte x = 0; x < validBeats; x++) beatAvg += rates[x];
      beatAvg /= validBeats;

      heartBeat = 255;

      Serial.print("[BPM] instant="); Serial.print((int)beatsPerMinute);
      Serial.print(" avg="); Serial.println(beatAvg);
    }
  }

  // ========================================================
  // SPO2 LOGIC
  // ========================================================
  switch (spo2State) {

    case FILLING:
      redBuffer[fillIndex] = redValue;
      irBuffer[fillIndex]  = irValue;
      fillIndex++;
      if (fillIndex >= 100) {
        Serial.println("[SpO2] Buffer full -> CALCULATING");
        spo2State = CALCULATING;
      }
      break;

    case COLLECTING:
      redBuffer[collectIdx] = redValue;
      irBuffer[collectIdx]  = irValue;
      collectIdx++;
      if (collectIdx >= 100) {
        Serial.println("[SpO2] 25 new samples -> CALCULATING");
        spo2State = CALCULATING;
      }
      break;

    case WAITING:
      // Just consume the sample, don't store it
      // Heart rate detection still happens above
      break;

    default:
      break;
  }
}

// ========================================
// DISPLAY RENDER
// ========================================
void render(bool finger) {
  u8g2.firstPage();
  do {
    if (finger) {
      showMeasurements();
    } else {
      showLanding();
    }
  } while (u8g2.nextPage());
}

// ========================================
// LANDING PAGE
// ========================================
void showLanding() {
  int cx = 64, cy = 26;

  u8g2.drawDisc(cx - 6, cy - 4, 6);
  u8g2.drawDisc(cx + 6, cy - 4, 6);
  u8g2.drawBox(cx - 6, cy - 4, 12, 8);
  for (int i = 0; i < 12; i++) {
    u8g2.drawHLine(cx - 6 + i / 2, cy + 4 + i / 2, 12 - i);
  }

  int pulseX = (heartScale * 6) - 20;
  u8g2.setDrawColor(0);
  if (pulseX > cx - 20) {
    u8g2.drawLine(cx - 20, cy, pulseX - 8, cy);
  } else {
    u8g2.drawLine(cx - 20, cy, cx + 20, cy);
  }
  if (pulseX >= cx - 20 && pulseX <= cx + 20) {
    u8g2.drawLine(pulseX - 8, cy,      pulseX - 6, cy - 10);
    u8g2.drawLine(pulseX - 6, cy - 10, pulseX - 4, cy + 8);
    u8g2.drawLine(pulseX - 4, cy + 8,  pulseX - 2, cy);
    if (pulseX < cx + 12) u8g2.drawLine(pulseX - 2, cy, cx + 20, cy);
  }

  u8g2.setDrawColor(1);
  u8g2.setFont(u8g2_font_helvB10_tr);
  u8g2.drawStr(20, 58, "Place Finger");
}

// ========================================
// MEASUREMENT PAGE
// ========================================
void showMeasurements() {
  // Top bar
  u8g2.drawBox(0, 0, 128, 13);
  u8g2.setDrawColor(0);
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(8, 10, "BPM");
  u8g2.drawStr(88, 10, "SpO2");
  u8g2.setDrawColor(1);

  u8g2.drawVLine(64, 14, 50);

  // ======== LEFT: BPM ========
  u8g2.setFont(u8g2_font_open_iconic_all_2x_t);
  u8g2.drawGlyph(2, 34, heartBeat > 128 ? 0x50 : 0x4F);

  if (beatAvg > 0) {
    char bpmStr[4];
    sprintf(bpmStr, "%d", beatAvg);
    if (beatAvg >= 100) {
      u8g2.setFont(u8g2_font_logisoso20_tn);
      u8g2.drawStr(18, 48, bpmStr);
    } else {
      u8g2.setFont(u8g2_font_logisoso26_tn);
      u8g2.drawStr(20, 50, bpmStr);
    }
    u8g2.setFont(u8g2_font_5x8_tr);
    u8g2.drawStr(22, 60, "BPM");
  } else {
    showWaitingIndicator(8, 38, 50);
  }

  // ======== RIGHT: SpO2 ========
  if (validSPO2 && spo2 >= 70 && spo2 <= 100) {
    char spo2Str[4];
    sprintf(spo2Str, "%d", spo2);
    u8g2.setFont(u8g2_font_logisoso26_tn);
    int strWidth = u8g2.getStrWidth(spo2Str);
    int xPos = 68 + (58 - strWidth - 10) / 2;
    u8g2.drawStr(xPos, 50, spo2Str);
    u8g2.setFont(u8g2_font_helvB10_tr);
    u8g2.drawStr(xPos + strWidth + 2, 42, "%");
    u8g2.setFont(u8g2_font_5x8_tr);
    u8g2.drawStr(86, 60, "SpO2");
  } else {
    showWaitingIndicator(72, 38, 50);
  }

  // ---- Bottom status ----
  if (spo2State == FILLING) {
    u8g2.setFont(u8g2_font_5x8_tr);
    u8g2.drawStr(1, 63, "Calibrating");
    int progress = map(fillIndex, 0, 100, 0, 56);
    u8g2.drawFrame(68, 56, 58, 7);
    u8g2.drawBox(69, 57, progress, 5);
  } else if (beatAvg == 0 || !validSPO2) {
    u8g2.setFont(u8g2_font_5x8_tr);
    u8g2.drawStr(1, 63, "Measuring");
    int dots = ((millis() / 400) % 4);
    for (int i = 0; i < dots; i++) u8g2.drawStr(46 + i * 4, 63, ".");
  } else {
    unsigned long elapsed = (millis() - measureStartTime) / 1000;
    u8g2.drawFrame(0, 62, 128, 2);
    u8g2.drawBox(0, 62, min((int)(elapsed * 2), 128), 2);
  }
}

// ========================================
// WAITING ANIMATION
// ========================================
void showWaitingIndicator(int x, int y, int width) {
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(x + 4, y, "---");
  int dots = ((millis() / 400) % 4);
  for (int i = 0; i < dots; i++) u8g2.drawDisc(x + 12 + i * 8, y + 12, 2);
}
