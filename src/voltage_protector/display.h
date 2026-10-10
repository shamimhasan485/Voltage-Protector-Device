#pragma once

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

extern LiquidCrystal_I2C lcd;

void lcdLine(int row, const char *text);
void showHome();
void showMenu();
