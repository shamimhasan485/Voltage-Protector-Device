#include "display.h"
#include "config.h"
#include "settings.h"
#include "protection.h"
#include "buttons.h"

LiquidCrystal_I2C lcd(0x27, 16, 2);

// LCD  Part 16x2

void lcdLine(int row, const char *text) {
  char buf[17];
  snprintf(buf, sizeof(buf), "%-16s", text);  // pad, clears old chars
  lcd.setCursor(0, row);
  lcd.print(buf);
}

void showHome() {
  char line[32];

  snprintf(line, sizeof(line), "%3.0fV %4.1fA %s", volt, amp, state == NORMAL ? "ON" : "OFF");
  lcdLine(0, line);

  if (state == NORMAL) {
    lcdLine(1, "Status: NORMAL");
  } else if (state == LOCKED) {
    lcdLine(1, "OVERLOAD UP=RST");
  } else if (volt > cfg.overVolt - HYST) {
    lcdLine(1, "HIGH VOLTAGE");
  } else if (volt < cfg.underVolt + HYST) {
    lcdLine(1, "LOW VOLTAGE");
  } else {
    long left = cfg.delaySec - (long)((millis() - okSince) / 1000);
    snprintf(line, sizeof(line), "ON IN %lds", left > 0 ? left : 0);
    lcdLine(1, line);
  }
}

void showMenu() {
  const char *names[] = { "", "Over Volt", "Under Volt", "Over Current", "Reconnect" };
  char line[32];

  snprintf(line, sizeof(line), "%d/4 %s", menuPage, names[menuPage]);
  lcdLine(0, line);

  if (menuPage == 1) snprintf(line, sizeof(line), "> %.0f V", cfg.overVolt);
  if (menuPage == 2) snprintf(line, sizeof(line), "> %.0f V", cfg.underVolt);
  if (menuPage == 3) snprintf(line, sizeof(line), "> %.1f A", cfg.overAmp);
  if (menuPage == 4) snprintf(line, sizeof(line), "> %d s", cfg.delaySec);
  lcdLine(1, line);
}
