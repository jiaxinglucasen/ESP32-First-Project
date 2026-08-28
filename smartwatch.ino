#include <TFT_eSPI.h>
#include <Wire.h>
#include "RTClib.h"

// Hardware Pin Definitions
#define BATTERY_PIN    34  // Verified ADC pin for battery level
#define LED_PIN        26  // Flashlight Output
#define LASER_PIN      25  // Laser Control (To 1k Resistor -> Transistor Base)

#define BTN_FLASH     35  // TTGO Onboard Top Button
#define BTN_LASER      0  // TTGO Onboard Bottom Button

TFT_eSPI tft = TFT_eSPI();
RTC_DS3231 rtc;

// Toggle States
bool flashState = false;
bool laserState = false;

// Debounce Tracking
unsigned long lastDebounceFlash = 0;
unsigned long lastDebounceLaser = 0;
const unsigned long debounceDelay = 200; 

void setup() {
  Serial.begin(115200);

  // Initialize Pin Modes
  pinMode(LED_PIN, OUTPUT);
  pinMode(LASER_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(LASER_PIN, LOW);

  pinMode(BTN_FLASH, INPUT_PULLUP);
  pinMode(BTN_LASER, INPUT_PULLUP);

  analogReadResolution(12);

  // Initialize Display
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  // Initialize RTC
  Wire.begin(21, 22); // SDA = GPIO 21, SCL = GPIO 22
  if (!rtc.begin()) {
    tft.setTextColor(TFT_RED);
    tft.drawString("RTC Error!", 10, 10, 2);
  }

  // Set time with a +60 second offset to compensate for upload delay
  // (COMMENT OUT line 51 after uploading once so time doesn't reset on reboot)
  DateTime compileTime = DateTime(F(__DATE__), F(__TIME__));
  //rtc.adjust(DateTime(compileTime.unixtime() + 60)); 
}

void loop() {
  handleButtons();
  updateDisplay();
  delay(100);
}

void handleButtons() {
  unsigned long currentMillis = millis();

  // Read Top Onboard Button (Flashlight Toggle)
  if (digitalRead(BTN_FLASH) == LOW) {
    if (currentMillis - lastDebounceFlash > debounceDelay) {
      flashState = !flashState;
      digitalWrite(LED_PIN, flashState ? HIGH : LOW);
      lastDebounceFlash = currentMillis;
    }
  }

  // Read Bottom Onboard Button (Laser Toggle)
  if (digitalRead(BTN_LASER) == LOW) {
    if (currentMillis - lastDebounceLaser > debounceDelay) {
      laserState = !laserState;
      digitalWrite(LASER_PIN, laserState ? HIGH : LOW);
      lastDebounceLaser = currentMillis;
    }
  }
}

void updateDisplay() {
  // 1. Read Battery & Charging Status
  uint32_t rawSum = 0;
  for (int i = 0; i < 10; i++) {
    rawSum += analogRead(BATTERY_PIN);
    delay(1);
  }
  float rawAnalog = rawSum / 10.0;
  float voltage = (rawAnalog / 4095.0) * 3.3 * 2.0;

  bool isCharging = (voltage > 4.25);
  int batteryPct = constrain((int)(((voltage - 3.2) / (4.2 - 3.2)) * 100.0), 0, 100);

  // 2. Clear Display Canvas
  tft.fillScreen(TFT_BLACK);

  // Outer Border
  tft.drawRoundRect(2, 2, tft.width() - 4, tft.height() - 4, 8, TFT_BLUE);

  // Header: Battery & Charge Status
  tft.setTextSize(1);
  if (isCharging) {
    tft.setTextColor(TFT_YELLOW);
    tft.drawString("CHARGING", 10, 10, 2);
  } else {
    tft.setTextColor(batteryPct > 20 ? TFT_GREEN : TFT_RED);
    tft.drawString("BAT: " + String(batteryPct) + "%", 10, 10, 2);
  }

  // Status Indicators for LED / Laser
  tft.setTextColor(flashState ? TFT_WHITE : TFT_DARKGREY);
  tft.drawString("LED", 160, 10, 2);

  tft.setTextColor(laserState ? TFT_RED : TFT_DARKGREY);
  tft.drawString("LASER", 195, 10, 2);

  // 3. Time Display
  DateTime now = rtc.now();
  char timeBuf[9];
  snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d", now.hour(), now.minute(), now.second());

  tft.setTextColor(TFT_CYAN);
  tft.drawString(timeBuf, 25, 45, 6); // Large Font

  // 4. Date Display
  char dateBuf[12];
  snprintf(dateBuf, sizeof(dateBuf), "%04d-%02d-%02d", now.year(), now.month(), now.day());

  tft.setTextColor(TFT_WHITE);
  tft.drawString(dateBuf, 65, 100, 2);
}
