#pragma once

#include <Arduino.h>

// ---------- buttons ----------
struct Button {
  uint8_t pin;
  bool pressed;
  bool lastRead;
  unsigned long changedAt;
};

extern int menuPage;
extern bool menuDirty;

bool pressed(Button &b);
void readButtons();
