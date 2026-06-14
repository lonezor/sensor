#include <SPI.h>
#include <Wire.h>
#include <hardware/watchdog.h>

#include "Adafruit_LEDBackpack.h"
#include <Adafruit_GFX.h>

#include <Adafruit_INA260.h>

#include "CH9120.h"

UCHAR CH9120_LOCAL_IP[4] = {192, 168, 1, 30};
UCHAR CH9120_GATEWAY[4] = {192, 168, 1, 1};
UCHAR CH9120_SUBNET_MASK[4] = {255, 255, 255, 0};
UCHAR CH9120_REMOTE_IP[4] = {192, 168, 1, 155};
UWORD CH9120_LOCAL_PORT = 30;
UWORD CH9120_REMOTE_PORT = 30;

Adafruit_INA260 ina260;

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

#define PIN_VOLTAGE_BTN (8)
#define PIN_CURRENT_BTN (9)

Adafruit_7segment led_display_00 = Adafruit_7segment();
Adafruit_7segment led_display_01 = Adafruit_7segment();

volatile int display_data_0[5];
volatile int display_data_1[5];

static uint32_t display_ref_ts = 0;
static uint64_t iterations = 0;

int prev_display_data_0[5];
int prev_display_data_1[5];

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

void update_display(Adafruit_7segment *seven_segment_display,
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
  Serial.println("Initialization Starting");

 // rp2040.wdt_begin(8000);

 // if (watchdog_caused_reboot()) {
 //   Serial.println("*** WATCHDOG REBOOT DETECTED ***");
 // }

  pinMode(PIN_VOLTAGE_BTN, INPUT_PULLUP);  
  pinMode(PIN_CURRENT_BTN, INPUT_PULLUP);

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

  led_display_00.begin(0x70);

  led_display_00.setBrightness(14); // 5V power supply

  Wire1.setSDA(2);
  Wire1.setSCL(3);
  Wire1.begin();

  led_display_01.begin(0x71, &Wire1);

  led_display_01.setBrightness(14);

  if (!ina260.begin(0x40, &Wire1)) {
    Serial.println("INA260 not found on Wire1, check wiring!");
    while (1) {
      delay(10);
    }
  }

  analogReadResolution(12);

  CH9120_init(CH9120_LOCAL_IP, CH9120_GATEWAY, CH9120_SUBNET_MASK,
              CH9120_REMOTE_IP, CH9120_LOCAL_PORT, CH9120_REMOTE_PORT);

  Serial.println("Initialization Done");
}

void setup1() {
  
}

void loop1() {
  delay(1000);
}

void loop() {
  //rp2040.wdt_reset();

  int voltage_btn = digitalRead(PIN_VOLTAGE_BTN);
  int current_btn = digitalRead(PIN_CURRENT_BTN);


 float current_mA = ina260.readCurrent();
 if (current_mA < 0) {
   current_mA = 0;
 }
 if (current_mA > 9999) {
   current_mA = 9999;
 }

 int current_part_a = (int)(current_mA / 1000.0f);
 int current_part_b = (int)(current_mA - (current_part_a * 1000) + 0.5f);

 if (current_part_b >= 1000) {
   current_part_b = 0;
   current_part_a++;
 }

  float voltage = ina260.readBusVoltage() / 1000.0f;
  if (voltage < 0) {
    voltage = 0;
  }
  if (voltage > 36) {
    voltage = 36; 
  }
  
  int voltage_part_a = (int)voltage;
  int voltage_part_b = (int)((voltage - voltage_part_a) * 100.0f + 0.5f);

  
  if (voltage_part_b >= 100) {
    voltage_part_b = 0;
    voltage_part_a++;
  }

  char display_data[128];
  
  uint32_t display_elapsed = (uint32_t)millis() - display_ref_ts;

  char voltage_section[32];
  char current_section[32];
  snprintf(voltage_section, sizeof(voltage_section), "D_0=aaaae");
  snprintf(current_section, sizeof(current_section), "D_1=aaaae");

  if (voltage_btn == 0) {
    if (voltage_part_a > 99) {
      voltage_part_a = 99;
      voltage_part_b = 99;
    }
    snprintf(voltage_section, sizeof(voltage_section),
                 "D_0=%02d%02dc", voltage_part_a, voltage_part_b);
  }

  if (current_btn == 0) {
    if (current_part_a > 9) {
      current_part_a = 9;
      current_part_b = 999;
      
    }
    snprintf(current_section, sizeof(current_section),
                 "D_1=%01d%03db", current_part_a, current_part_b);
  }
  

  snprintf(display_data, sizeof(display_data), "%s;%s\n", voltage_section, current_section);

  parse_latest_stats(display_data);

  if (display_elapsed >= 250) {
    update_display(&led_display_00, display_data_0, prev_display_data_0);
    update_display(&led_display_01, display_data_1, prev_display_data_1);

    display_ref_ts = (uint32_t)millis();
  }

  char msg[512];
  snprintf(msg, sizeof(msg), 
         "SOLAR_POWER_DC_SUPPLY: voltage_display='%s', 'current_display='%s', voltage=%02u.%02uV, current=%u.%03uA, v_btn=%d, c_btn=%d, sample_idx=%010" PRIu64, 
         voltage_section,
         current_section,
         (unsigned)voltage_part_a, 
         (unsigned)voltage_part_b, 
         (unsigned)current_part_a, 
         (unsigned)current_part_b, 
         voltage_btn == 0,
         current_btn == 0,
         iterations);
  Serial.println(msg);

  SendUdpPacket(msg);

  iterations++;

  delay(100);
}
