#include "buttons.h"
#include "config.h"
#include "settings.h"
#include "protection.h"

Button btnMenu = { PIN_BTN_MENU, false, false, 0 };
Button btnUp = { PIN_BTN_UP, false, false, 0 };
Button btnDown = { PIN_BTN_DOWN, false, false, 0 };

int menuPage = 0;  // 0 = home, 1..4 = settings
bool menuDirty = false;

// Buttons and menu
bool pressed(Button &b) {
  bool now = (digitalRead(b.pin) == LOW);
  if (now != b.lastRead) {
    b.lastRead = now;
    b.changedAt = millis();
  }
  if (millis() - b.changedAt > 30 && now != b.pressed) {
    b.pressed = now;
    return now;
  }
  return false;
}

void readButtons() {
  if (pressed(btnMenu)) {
    menuPage = (menuPage + 1) % 5;
    if (menuPage == 0 && menuDirty) {
      saveConfig();
      menuDirty = false;
      Serial.print("Saved from button: ");
      printConfig();
    }
  }

  int step = 0;
  if (pressed(btnUp)) step = 1;
  if (pressed(btnDown)) step = -1;
  if (step == 0) return;

  if (menuPage == 0) {
    if (step == 1 && state == LOCKED) resetLock("button");
    return;
  }

  Config c = cfg;
  if (menuPage == 1) c.overVolt += step * 5;
  if (menuPage == 2) c.underVolt += step * 5;
  if (menuPage == 3) c.overAmp += step * 0.5;
  if (menuPage == 4) c.delaySec += step;
  if (checkConfig(c) == NULL) {  // ignore steps past the limits
    cfg = c;
    menuDirty = true;
  }
}
