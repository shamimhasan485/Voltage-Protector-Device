#include "commands.h"
#include "settings.h"
#include "protection.h"

void printHelp() {
  Serial.println("Commands:");
  Serial.println("  STATUS");
  Serial.println("  SET OV <200-300>     over voltage (V)");
  Serial.println("  SET UV <100-220>     under voltage (V)");
  Serial.println("  SET OC <0.5-20>      over current (A)");
  Serial.println("  SET DELAY <1-300>    reconnect delay (s)");
  Serial.println("  RESET                clear over current lock");
  Serial.println("  DEFAULTS");
}

void printStatus() {
  const char *s = (state == NORMAL) ? "NORMAL" : (state == LOCKED) ? "LOCKED"
                                                                   : "WAITING";
  Serial.printf("V=%.1fV  I=%.2fA  LOAD=%s  STATE=%s  LAST FAULT=%s\n",
                volt, amp, state == NORMAL ? "ON" : "OFF", s, lastFault);
  printConfig();
}

void handleCommand(String cmd) {
  cmd.trim();
  cmd.toUpperCase();
  if (cmd.length() == 0) return;
  Serial.println("> " + cmd);

  if (cmd == "HELP") {
    printHelp();
  } else if (cmd == "STATUS") {
    printStatus();
  } else if (cmd == "RESET") {
    resetLock("PC");
  } else if (cmd == "DEFAULTS") {
    cfg = DEFAULT_CFG;
    saveConfig();
    printConfig();
  } else if (cmd.startsWith("SET ")) {
    int sp = cmd.indexOf(' ', 4);
    if (sp < 0) {
      Serial.println("ERR use: SET <OV|UV|OC|DELAY> <value>");
      return;
    }
    String key = cmd.substring(4, sp);
    String val = cmd.substring(sp + 1);
    float v = val.toFloat();
    if (v == 0 && val != "0") {
      Serial.println("ERR value is not a number");
      return;
    }

    Config c = cfg;
    if (key == "OV") c.overVolt = v;
    else if (key == "UV") c.underVolt = v;
    else if (key == "OC") c.overAmp = v;
    else if (key == "DELAY") c.delaySec = (uint16_t)v;
    else {
      Serial.println("ERR unknown setting " + key);
      return;
    }

    const char *err = checkConfig(c);
    if (err) {
      Serial.printf("ERR %s (not changed)\n", err);
      return;
    }
    cfg = c;
    saveConfig();
    Serial.print("OK saved: ");
    printConfig();
  } else {
    Serial.println("ERR unknown command, type HELP");
  }
}

String rxLine;

void readSerial() {
  while (Serial.available()) {
    char ch = Serial.read();
    if (ch == '\n' || ch == '\r') {
      handleCommand(rxLine);
      rxLine = "";
    } else if (rxLine.length() < 40) {
      rxLine += ch;
    }
  }
}
