#include "protection.h"
#include "config.h"
#include "settings.h"

State state = WAITING;

float volt = 0, amp = 0;
const char *lastFault = "NONE";
unsigned long okSince = 0;  // voltage is ok since (ms)
unsigned long beepUntil = 0;

float readAvg(int pin) {
  long sum = 0;
  for (int i = 0; i < 16; i++) sum += analogRead(pin);
  return sum / 16.0;
}

void readSensors() {
  volt = readAvg(PIN_VOLT) * VOLT_MAX / 4095.0;
  amp = readAvg(PIN_AMP) * AMP_MAX / 4095.0;
}

void setLoad(bool on) {
  digitalWrite(PIN_RELAY, on ? RELAY_ON : RELAY_OFF);
  digitalWrite(PIN_LED_OK, on);
  digitalWrite(PIN_LED_FAULT, !on);
}

void trip(const char *reason, State next) {
  setLoad(false);
  state = next;
  lastFault = reason;
  okSince = millis();
  beepUntil = millis() + 400;
  Serial.printf("[%lu] TRIP %s  V=%.1f I=%.2f -> LOAD OFF\n", millis(), reason, volt, amp);
  if (next == LOCKED) Serial.println("Over current locked. Type RESET or press UP.");
}

void protect() {
  if (state == NORMAL) {
    if (volt > cfg.overVolt) trip("OVER VOLTAGE", WAITING);
    else if (volt < cfg.underVolt) trip("UNDER VOLTAGE", WAITING);
    else if (amp > cfg.overAmp) trip("OVER CURRENT", LOCKED);
    return;
  }

  if (state == WAITING) {
    bool voltOk = (volt <= cfg.overVolt - HYST) && (volt >= cfg.underVolt + HYST);
    if (!voltOk) {
      okSince = millis();  // start counting again
    } else if (millis() - okSince >= cfg.delaySec * 1000UL) {
      state = NORMAL;
      setLoad(true);
      Serial.printf("[%lu] RECONNECT  V=%.1f -> LOAD ON\n", millis(), volt);
    }
  }
  // LOCKED: wait for reset
}

void resetLock(const char *from) {
  if (state != LOCKED) {
    Serial.println("Not locked");
    return;
  }
  state = WAITING;
  okSince = millis();
  Serial.printf("Lock reset (%s), reconnecting after delay\n", from);
}
