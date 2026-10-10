#pragma once

#include <Arduino.h>

// ---------- state ----------
enum State { WAITING,
             NORMAL,
             LOCKED };

extern State state;
extern float volt, amp;
extern const char *lastFault;
extern unsigned long okSince;
extern unsigned long beepUntil;

float readAvg(int pin);
void readSensors();
void setLoad(bool on);
void trip(const char *reason, State next);
void protect();
void resetLock(const char *from);
