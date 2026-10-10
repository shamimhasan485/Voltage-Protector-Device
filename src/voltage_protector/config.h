// ---------- pins Configuration ----------
#define PIN_VOLT      35    // pot -> grid voltage
#define PIN_AMP       34    // pot -> load current
#define PIN_LED_FAULT 33    // red
#define PIN_LED_OK    25    // green
#define PIN_BUZZER    26
#define PIN_RELAY     27
#define PIN_BTN_DOWN  19
#define PIN_BTN_UP    18
#define PIN_BTN_MENU  5

#define RELAY_ON  HIGH      
#define RELAY_OFF LOW


#define VOLT_MAX  440.0
#define AMP_MAX   20.0
#define HYST      5.0       

// ---------- EEPROM ----------
#define EEPROM_SIZE  64
#define EEPROM_ADDR  0
#define CFG_MAGIC    0xA5   