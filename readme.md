# Voltage Protector using ESP32

An ESP32 watches the grid voltage and the load current. If the voltage goes too high or too low, or the current goes too high, it turns the relay off and the appliance is disconnected. You can change the limits with the buttons on the device or from the PC.

No mains is used here. Two potentiometers act as the voltage and current sensors and a 5V lamp / LED acts as the appliance.

## Folders

```
src/voltage_protector/   ESP32 code (open voltage_protector.ino)
src/pc_gui/vp_gui.py     PC app to see values and change settings
schematics/              block diagram, wiring diagram, Wokwi circuit
project_files/wokwi/     diagram.json for Wokwi
test_evidence/           test plan, logs, screenshots
design_document.pdf      one page report
```

## Run in Wokwi

1. Open https://wokwi.com/projects/new/esp32
2. Paste `voltage_protector.ino` in the sketch.ino tab.
3. Add the other files (`config.h`, `settings.h/.cpp`, `protection.h/.cpp`, `commands.h/.cpp`, `buttons.h/.cpp`, `display.h/.cpp`) with the **+** next to the tabs. Keep the same file names.
4. Paste `project_files/wokwi/diagram.json` in the diagram.json tab.
5. Library Manager -> add **LiquidCrystal I2C**.
6. Press Play.

The top slider is the grid voltage (middle is about 220V), the second slider is the load current. Type commands in the Serial Monitor at the bottom.

## Run on a real ESP32

Parts: ESP32 DevKit, 2 x 10k pot, 5V relay module, 16x2 LCD with I2C backpack, 3 push buttons, red + green LED with 220R, buzzer, a small 5V lamp or LED for the appliance.

| ESP32 pin | Connect to |
|---|---|
| GPIO35 | voltage pot middle pin (other legs 3V3 and GND) |
| GPIO34 | current pot middle pin (other legs 3V3 and GND) |
| GPIO27 | relay IN (VCC 5V, GND GND) |
| GPIO21 / 22 | LCD SDA / SCL (VCC 5V, GND GND) |
| GPIO5 / 18 / 19 | MENU / UP / DOWN button, other side to GND |
| GPIO25 | green LED + 220R |
| GPIO33 | red LED + 220R |
| GPIO26 | buzzer |

Don't connect the pot to 5V, the ESP32 ADC pins take max 3.3V.

1. Arduino IDE -> Boards Manager -> install **esp32 by Espressif**.
2. Library Manager -> install **LiquidCrystal I2C**.
3. Open `src/voltage_protector/voltage_protector.ino` (all files open as tabs), select **ESP32 Dev Module**, upload.
4. Serial Monitor at **115200**, line ending **Newline**.

If the LCD shows nothing, turn the contrast pot on the back or change the address `0x27` to `0x3F`. If the relay works the opposite way, swap `RELAY_ON` / `RELAY_OFF` in `config.h`.

## Buttons

- Home screen: shows V, A and status. UP resets an over current lock.
- MENU goes through Over Volt -> Under Volt -> Over Current -> Reconnect -> back home.
- UP / DOWN change the value. It is saved to EEPROM when you come back to home.

## Serial commands

```
HELP
STATUS
SET OV 245       over voltage    200-300 V
SET UV 185       under voltage   100-220 V
SET OC 8.5       over current    0.5-20 A
SET DELAY 10     reconnect delay 1-300 s
RESET            clear over current lock
DEFAULTS         250V / 180V / 10A / 5s
```

## PC app

```
pip install pyserial
python src/pc_gui/vp_gui.py
```

Select the COM port and press Connect. It shows voltage, current, load ON/OFF and lets you change the settings. Close the Arduino Serial Monitor first, only one program can use the port. If you get `No module named 'serial.tools'`, run `pip uninstall serial` and install `pyserial` again.

## Default limits

| | Default | What happens |
|---|---|---|
| Over voltage | 250V | relay off, comes back on after the voltage is ok for 5s |
| Under voltage | 180V | same |
| Over current | 10A | relay off and locked, needs RESET or UP button |
