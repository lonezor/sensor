#include <RadioLib.h>
#include <SPI.h>
#include <Wire.h>
#include <hardware/watchdog.h>

#include "Adafruit_LEDBackpack.h"
#include <Adafruit_GFX.h>

#define DISPLAY_FLAG_00 (1 << 0)
#define DISPLAY_FLAG_01 (1 << 1)
#define DISPLAY_FLAG_02 (1 << 2)
#define DISPLAY_FLAG_03 (1 << 3)
#define DISPLAY_FLAG_04 (1 << 4)
#define DISPLAY_FLAG_05 (1 << 5)
#define DISPLAY_FLAG_06 (1 << 6)
#define DISPLAY_FLAG_07 (1 << 7)
#define DISPLAY_FLAG_08 (1 << 8)
#define DISPLAY_FLAG_09 (1 << 9)
#define DISPLAY_FLAG_10 (1 << 10)
#define DISPLAY_FLAG_11 (1 << 11)

#define LORA_SS 13
#define LORA_DIO1 16
#define LORA_RST 23
#define LORA_BUSY 18
#define LORA_ANT_SW 17

#define PIN_PIEZO (12)
#define PIN_POWER_BTN (11)
#define PIN_SEND_BTN (10)
#define PIN_LED (9)

Adafruit_7segment led_display_00 = Adafruit_7segment();
Adafruit_7segment led_display_01 = Adafruit_7segment();

volatile int display_data_0[5];
volatile int display_data_1[5];

static uint32_t display_ref_ts = 0;

int prev_display_data_0[5];
int prev_display_data_1[5];

// CORRECT v7.5.1 declaration
Module radio(LORA_SS, LORA_DIO1, LORA_RST, LORA_BUSY, SPI1);
SX1262 lora(&radio);

typedef enum {
  FREQUENCY_MODE_OFF,
  FREQUENCY_MODE_RTT_MODULATED_BASELINE,
  FREQUENCY_MODE_RTT_MODULATED_FREQUENCY,
} frequency_modes_t;

const int PIEZO_FREQUENCIES_RTT[16] = {0,   20,  40,  80,  120, 160, 200, 240,
                                       280, 320, 360, 400, 440, 480, 520, 0};

const int PIEZO_FREQUENCIES_RSSI[105] = {
    // 105 RSSI mappings
    3900, 3863, 3825, 3788, 3751, 3713, 3676, 3639, 3602, 3564, 3527, 3490,
    3452, 3415, 3378, 3340, 3303, 3266, 3228, 3191, 3154, 3117, 3079, 3042,
    3005, 2967, 2930, 2893, 2855, 2818, 2781, 2743, 2706, 2669, 2632, 2594,
    2557, 2520, 2482, 2445, 2408, 2370, 2333, 2296, 2258, 2221, 2184, 2147,
    2109, 2072, 2035, 1997, 1960, 1923, 1885, 1848, 1811, 1773, 1736, 1699,
    1662, 1624, 1587, 1550, 1512, 1475, 1438, 1400, 1363, 1326, 1288, 1251,
    1214, 1177, 1139, 1102, 1065, 1027, 990,  953,  915,  878,  841,  803,
    766,  729,  692,  654,  617,  580,  542,  505,  468,  430,  393,  356,
    318,  281,  244,  207,  169,  132,  95,   57,   20,
};

#define ADC_MAX_OBSERVED 3800
#define ADC_POTENTIOMETER_STEP_SIZE (ADC_MAX_OBSERVED / 16)

typedef struct {
  frequency_modes_t mode;
  int frequency;
} piezo_config_t;

static piezo_config_t piezo_config;
static bool leds_active = false;
static bool send_signal = false;
static unsigned long lastPressTime = 0;

static volatile int rtt = 0;
static volatile int rssi = 0;

static int ascii_to_integer(char c) {
  switch (c) {
  case '0':
    return 0x0;
  case '1':
    return 0x1;
  case '2':
    return 0x2;
  case '3':
    return 0x3;
  case '4':
    return 0x4;
  case '5':
    return 0x5;
  case '6':
    return 0x6;
  case '7':
    return 0x7;
  case '8':
    return 0x8;
  case '9':
    return 0x9;
  case 'a':
    return 0xa;
  case 'b':
    return 0xb;
  case 'c':
    return 0xc;
  case 'd':
    return 0xd;
  case 'e':
    return 0xe;
  case 'f':
    return 0xf;
  default:
    return 0xa;
  }
}

static void update_stats(volatile int display_data[5], char *value_str) {
  int len = strlen(value_str);
  if (len == 5) {
    for (int i = 0; i < len; i++) {
      display_data[4 - i] = ascii_to_integer(value_str[i]);
    }
  }
}

static void parse_latest_stats(char *input_str) {
  // D_0=aaaae;D_1=aaaae;D_2=aaaae;D_3=aaaae;D_4=aaaae;D_5=aaaae\n

  const char *pair_delimiters = ";\n";

  char *pair_token = strtok(input_str, pair_delimiters);

  int idx = 0;

  while (pair_token != NULL) {
    char *equals_sign = strchr(pair_token, '=');

    if (equals_sign != NULL) {
      char *value = equals_sign + 1;

      switch (idx) {
      case 0:
        // D_0
        update_stats(display_data_0, value);
        break;
      case 1:
        // D_1
        update_stats(display_data_1, value);
        break;
      default:
        break;
      }

      idx++;
    }

    pair_token = strtok(NULL, pair_delimiters);
  }
}

static uint16_t identify_delta(volatile int display_data[5],
                               int prev_display_data[5]) {
  uint16_t bitfield = 0;

  for (int i = 0; i < 4; i++) {
    if (display_data[i + 1] != prev_display_data[i + 1]) {
      bitfield |= 1 << i;
    }
  }

  if (display_data[0] != prev_display_data[0]) {
    bitfield = 0xf;
  }

  return bitfield;
}

static void volatile_copy(int dst[5], volatile int src[5]) {
  // memcpy() should not be used
  for (int i = 0; i < 5; i++) {
    dst[i] = src[i];
  }
}

void loop1_update_display(Adafruit_7segment *seven_segment_display,
                          volatile int display_data[5],
                          int prev_display_data[5]) {
  uint16_t update_bf = identify_delta(display_data, prev_display_data);

  if (update_bf > 0) {
    volatile_copy(prev_display_data, display_data);
  }

  uint16_t d01_flags =
      DISPLAY_FLAG_00 | DISPLAY_FLAG_01 | DISPLAY_FLAG_02 | DISPLAY_FLAG_03;

  ////////////////////////////////////////////////////////////
  ///////// ZERO PADDING STATE (0xa indication) //////////////
  ////////////////////////////////////////////////////////////
  // Zero pad means modifying positions that only relay
  // structure, not any real information. Display turned off
  // or zero.

  if (display_data[4] == 0xa && update_bf & DISPLAY_FLAG_03) {
    seven_segment_display->writeDigitRaw(0, 0);
  }

  if (display_data[3] == 0xa && update_bf & DISPLAY_FLAG_02) {
    seven_segment_display->writeDigitRaw(1, 0);
  }

  if (display_data[2] == 0xa && update_bf & DISPLAY_FLAG_01) {
    seven_segment_display->writeDigitRaw(3, 0);
  }

  if (display_data[1] == 0xa && update_bf & DISPLAY_FLAG_00) {
    seven_segment_display->writeDigitRaw(4, 0);
  }

  if (update_bf & d01_flags) {
    seven_segment_display->writeDisplay();
  }

  //////////////////////////////////////////////////////////////
  ///////////// VALUE 0-9 STATE AND DECIMAL POINT //////////////
  //////////////////////////////////////////////////////////////

  if (display_data[4] != 0xa && update_bf & DISPLAY_FLAG_03) {
    if (display_data[0] == 0xb) {
      seven_segment_display->writeDigitNum(0, display_data[4], true);
    } else {
      seven_segment_display->writeDigitNum(0, display_data[4], false);
    }
  }

  if (display_data[3] != 0xa && update_bf & DISPLAY_FLAG_02) {
    if (display_data[0] == 0xc) {
      seven_segment_display->writeDigitNum(1, display_data[3], true);
    } else {
      seven_segment_display->writeDigitNum(1, display_data[3], false);
    }
  }

  if (display_data[2] != 0xa && update_bf & DISPLAY_FLAG_01) {
    if (display_data[0] == 0xd) {
      seven_segment_display->writeDigitNum(3, display_data[2], true);
    } else {
      seven_segment_display->writeDigitNum(3, display_data[2], false);
    }
  }

  if (display_data[1] != 0xa && update_bf & DISPLAY_FLAG_00) {
    // Forth decimal not used
    seven_segment_display->writeDigitNum(4, display_data[1], false);
  }
  if (update_bf & d01_flags) {
    seven_segment_display->writeDisplay();
  }
}

void setup() {
  Serial.begin(9600);

  // Watchdog configured due to observed stability issues
  // of the radio code. Edge cases that may freeze function calls
  // beyond 8s
  rp2040.wdt_begin(8000);

  if (watchdog_caused_reboot()) {
    Serial.println("*** WATCHDOG REBOOT DETECTED ***");
  }

  pinMode(LORA_ANT_SW, OUTPUT);
  digitalWrite(LORA_ANT_SW, HIGH);

  SPI1.setRX(24);
  SPI1.setTX(15);
  SPI1.setSCK(14);
  SPI1.begin(false);

  int state = lora.begin(868.0);
  Serial.print("Init: ");
  Serial.println(state == RADIOLIB_ERR_NONE ? "OK" : String(state));

  // Power control - exists on SX1262 class
  lora.setOutputPower(22);

  // 0xa means 7 segment display OFF (display zero when zero pad is active)
  for (int i = 0; i < 5; i++) {
    if (i == 0) {
      display_data_0[i] = 0xf;
      display_data_1[i] = 0xf;
    } else {
      display_data_0[i] = 0xa;
      display_data_1[i] = 0xa;
    }

    prev_display_data_0[i] = 0x0;
    prev_display_data_1[i] = 0x0;
  }

  Wire.setSDA(0);
  Wire.setSCL(1);
  Wire.begin();

  led_display_00.begin(0x71);

  led_display_00.setBrightness(4);

  Wire1.setSDA(2);
  Wire1.setSCL(3);
  Wire1.begin();

  led_display_01.begin(0x72, &Wire1);

  led_display_01.setBrightness(4);
}

void setup1() {
  pinMode(PIN_PIEZO, OUTPUT);
  pinMode(PIN_POWER_BTN, INPUT_PULLUP);
  pinMode(PIN_SEND_BTN, INPUT_PULLUP);
  pinMode(PIN_LED, OUTPUT);

  analogReadResolution(12);

  piezo_config.mode = FREQUENCY_MODE_OFF;
  piezo_config.frequency = 0;
}

// This core is dedicated to the piezo handling
void loop1() {
  int setting = analogRead(A0);
  setting = 4095 - setting; // invert it to make sense physically

  if (rtt < 50 || rssi == 0) {
    return;
  }

  int idx = setting / ADC_POTENTIOMETER_STEP_SIZE;
  if (idx > 15) {
    idx = 15;
  }

  if (idx == 0) {
    piezo_config.mode = FREQUENCY_MODE_OFF;
    piezo_config.frequency = 0;
  } else if (idx == 15) {
    piezo_config.mode = FREQUENCY_MODE_RTT_MODULATED_FREQUENCY;

    // Clamp to 15-120 range
    float abs_rssi = fabs(rssi);
    if (abs_rssi < 15.0)
      abs_rssi = 15.0;
    if (abs_rssi > 120.0)
      abs_rssi = 120.0;

    int index = (int)(((abs_rssi - 15.0) * 104.0 / 105.0) + 0.5);
    if (index < 0)
      index = 0;
    if (index > 104)
      index = 104;

    piezo_config.frequency = PIEZO_FREQUENCIES_RSSI[index];
  } else {
    piezo_config.mode = FREQUENCY_MODE_RTT_MODULATED_BASELINE;
    piezo_config.frequency = PIEZO_FREQUENCIES_RTT[idx];
  }

  const char *pmode = "INIT";

  if (piezo_config.mode == FREQUENCY_MODE_RTT_MODULATED_BASELINE) {
    pmode = "RTT_MODULATED_BASELINE";

    tone(PIN_PIEZO, piezo_config.frequency);
    delay(50);

    // In this main mode, we can support greater range of RTT
    tone(PIN_PIEZO, 0);
    delay(rtt);
  } else if (piezo_config.mode == FREQUENCY_MODE_RTT_MODULATED_FREQUENCY) {
    pmode = "RTT_MODULATED_FREQUENCY";

    tone(PIN_PIEZO, piezo_config.frequency);
    delay(50);
    tone(PIN_PIEZO, 0);
    delay(950);
  } else {
    pmode = "OFF";
    tone(PIN_PIEZO, 0);
    delay(50);
  }

  char msg[128];
  snprintf(msg, sizeof(msg),
           "[%s] idx %02d (raw %04d), rtt %04d, abs(rssi) %03d, frequency %04d",
           pmode, idx, setting, rtt, rssi, piezo_config.frequency);
  Serial.println(msg);

  delay(200);
}

// This core is radio and RTT identification plus overall state logic
void loop() {
  rp2040.wdt_reset();

  int power_btn = digitalRead(PIN_POWER_BTN);

  // This button controls the overall functionality. It is not actually a power
  // switch but controls behavior
  if (power_btn == LOW) {
    leds_active = true;
  } else {
    leds_active = false;
  }

  digitalWrite(PIN_LED, HIGH);

  if (digitalRead(PIN_SEND_BTN) == LOW) {
    if (millis() - lastPressTime >= 1000) {

      Serial.println("send_signal: true");
      send_signal = true;
      lastPressTime = millis();
    }
  }

  int voltage_divider_raw = analogRead(A1);
  const float R1 = 10000.0;
  const float R2 = 6800.0;
  const float VREF = 3.3;
  const int ADC_MAX = 4095;
  const float DIVIDER_RATIO = (R1 + R2) / R2;
  float vDivider = (voltage_divider_raw * VREF) / ADC_MAX;
  float vActual = vDivider * DIVIDER_RATIO;
  int voltage_part_a = (int)vActual;
  int voltage_part_b = (vActual - voltage_part_a) * 100;

  char seven_segment_config[128];

  uint32_t display_elapsed = (uint32_t)millis() - display_ref_ts;

  if (leds_active) {
    if (send_signal) {
      snprintf(seven_segment_config, sizeof(seven_segment_config),
               "D_0=a11ae;D_1=a11ae\n");
    } else {
      if (rssi == 0) {
        snprintf(seven_segment_config, sizeof(seven_segment_config),
                 "D_0=%02d%02dc;D_1=aaaae\n", voltage_part_a, voltage_part_b);
      } else {
        snprintf(seven_segment_config, sizeof(seven_segment_config),
                 "D_0=%02d%02dc;D_1=%04de\n", voltage_part_a, voltage_part_b,
                 rssi);
      }
    }
  } else {
    snprintf(seven_segment_config, sizeof(seven_segment_config),
             "D_0=aaaae;D_1=aaaae\n");
  }

  parse_latest_stats(seven_segment_config);

  if (display_elapsed > 0) {
    loop1_update_display(&led_display_00, display_data_0, prev_display_data_0);
    loop1_update_display(&led_display_01, display_data_1, prev_display_data_1);

    display_ref_ts = (uint32_t)millis();
  }

  if (send_signal) {
    digitalWrite(LORA_ANT_SW, LOW);
    Serial.println("TX LORA_ACTIVATE_COMMAND");
    lora.transmit("LORA_ACTIVATE_COMMAND");
    digitalWrite(LORA_ANT_SW, HIGH);
    send_signal = false;
  } else {
    unsigned long tStart = millis();

    digitalWrite(LORA_ANT_SW, LOW);
    lora.transmit("LORA_ECHO_REQUEST");
    digitalWrite(LORA_ANT_SW, HIGH);

    String str;

    // must be 5000 (not 2000) since the function blocks for ~30s if the other
    // radio dies in mid-flight due to power loss
    int state = lora.receive(str, 5000);

    if (state == RADIOLIB_ERR_NONE) {
      // Convert to C-string and check prefix exactly
      const char *cstr = str.c_str();
      if (strstr(cstr, "LORA_ECHO_RESPONSE") == cstr) { // starts exactly with
        rtt = (int)(millis() - tStart);
        // RSSI is better indicator on 7 segment display than RTT (that is
        // almost constant)
        float raw_rssi = lora.getRSSI();
        if (isnan(raw_rssi)) {
          rssi = 0;
        } else {
          rssi = abs(raw_rssi);
        }
      }
    } else {
      rtt = 0;
      rssi = 0;
    }
  }

  delay(200);
}
