#include "config.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

#include "settings.h"
#include "protection.h"
#include "commands.h"
#include "buttons.h"
#include "display.h"

unsigned long lastCheck = 0, lastLcd = 0;

void setup() {
  Serial.begin(115200);

  pinMode(PIN_RELAY, OUTPUT);
  pinMode(PIN_LED_OK, OUTPUT);
  pinMode(PIN_LED_FAULT, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  setLoad(false);  // load stays off until voltage is checked

  pinMode(PIN_BTN_MENU, INPUT_PULLUP);
  pinMode(PIN_BTN_UP, INPUT_PULLUP);
  pinMode(PIN_BTN_DOWN, INPUT_PULLUP);

  EEPROM.begin(EEPROM_SIZE);

  lcd.init();
  lcd.backlight();
  lcdLine(0, "VOLTAGE PROTECT.");
  lcdLine(1, "ESP32");

  Serial.println("\n=== Voltage Protector Device ===");
  loadConfig();
  printConfig();
  printHelp();
  okSince = millis();
}

void loop() {
  readSerial();
  readButtons();

  if (millis() - lastCheck >= 5) {  // check every 5ms
    lastCheck = millis();
    readSensors();
    protect();
  }

  digitalWrite(PIN_BUZZER, millis() < beepUntil);

  if (millis() - lastLcd >= 250) {
    lastLcd = millis();
    if (menuPage == 0) showHome();
    else showMenu();
  }
}
