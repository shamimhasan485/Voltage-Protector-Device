#pragma once

#include <Arduino.h>

struct Config {
  uint8_t magic;
  float overVolt;
  float underVolt;
  float overAmp;
  uint16_t delaySec;
};

extern Config cfg;
extern const Config DEFAULT_CFG;

const char *checkConfig(const Config &c);
void saveConfig();
void loadConfig();
void printConfig();
