#include "settings.h"
#include "config.h"
#include <EEPROM.h>

Config cfg;
const Config DEFAULT_CFG = { CFG_MAGIC, 250.0, 180.0, 10.0, 5 };

// returns NULL if ok, else the reason
const char *checkConfig(const Config &c) {
  if (c.overVolt < 200 || c.overVolt > 300) return "OV range 200-300V";
  if (c.underVolt < 100 || c.underVolt > 220) return "UV range 100-220V";
  if (c.overVolt - c.underVolt < 30) return "OV must be 30V above UV";
  if (c.overAmp < 0.5 || c.overAmp > 20) return "OC range 0.5-20A";
  if (c.delaySec < 1 || c.delaySec > 300) return "DELAY range 1-300s";
  return NULL;
}

void saveConfig() {
  cfg.magic = CFG_MAGIC;
  EEPROM.put(EEPROM_ADDR, cfg);
  EEPROM.commit();
}

void loadConfig() {
  EEPROM.get(EEPROM_ADDR, cfg);
  if (cfg.magic != CFG_MAGIC || checkConfig(cfg) != NULL) {
    Serial.println("EEPROM empty or bad, loading defaults");
    cfg = DEFAULT_CFG;
    saveConfig();
  }
}

void printConfig() {
  Serial.printf("OV=%.0fV  UV=%.0fV  OC=%.1fA  DELAY=%ds\n", cfg.overVolt, cfg.underVolt, cfg.overAmp, cfg.delaySec);
}
