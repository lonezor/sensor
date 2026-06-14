#include <SPI.h>
#include <Wire.h>

#include "Adafruit_SHTC3.h"
#include "CH9120.h"
#include "Adafruit_MAX31865.h"

// MAX31865: GP2 - GP5
#define UNIT_01_MAX_CS   2
#define UNIT_01_MAX_DI   3
#define UNIT_01_MAX_DO   4
#define UNIT_01_MAX_CLK  5

// MAX31865: GP6 - GP9
#define UNIT_02_MAX_CS   6
#define UNIT_02_MAX_DI   7
#define UNIT_02_MAX_DO   8
#define UNIT_02_MAX_CLK  9

#define RREF      4300.0   // Adafruit PT1000 board uses 4300 ohm reference
#define RNOMINAL  1000.0   // PT1000 nominal resistance

static UCHAR CH9120_LOCAL_IP[4] = {172, 16, 0, 101};
static UCHAR CH9120_GATEWAY[4] = {172, 16, 0, 1};
static UCHAR CH9120_SUBNET_MASK[4] = {255, 255, 252, 0};
static UCHAR CH9120_REMOTE_IP[4] = {172, 16, 0, 200};
static UWORD CH9120_LOCAL_PORT = 101;
static UWORD CH9120_REMOTE_PORT = 200;

static Adafruit_SHTC3 shtc3 = Adafruit_SHTC3();
static Adafruit_MAX31865 rtd_01 = Adafruit_MAX31865(UNIT_01_MAX_CS, UNIT_01_MAX_DI, UNIT_01_MAX_DO, UNIT_01_MAX_CLK);
static Adafruit_MAX31865 rtd_02 = Adafruit_MAX31865(UNIT_02_MAX_CS, UNIT_02_MAX_DI, UNIT_02_MAX_DO, UNIT_02_MAX_CLK);

static char msg[1024];

void setup() {
  Serial.begin(115200);

  // Configure the I2C pins for GP0 (SDA) and GP1 (SCL)
  Wire.setSDA(0);
  Wire.setSCL(1);
  Wire.begin();

  if (!shtc3.begin(&Wire)) {
    while (1) { 
      Serial.println("Couldn't find SHTC3 sensor!");
      delay(500);
    }
  }

  rtd_01.begin(MAX31865_3WIRE);
  rtd_02.begin(MAX31865_3WIRE);

  CH9120_init(CH9120_LOCAL_IP, CH9120_GATEWAY, CH9120_SUBNET_MASK,
              CH9120_REMOTE_IP, CH9120_LOCAL_PORT, CH9120_REMOTE_PORT);

  memset(msg, 0, sizeof(msg));
}

void setup1() {
  // Currently, there is no need to use this core
  delay(1000);
}

void loop() {
  sensors_event_t temp;
  sensors_event_t humidity;
  
  shtc3.getEvent(&humidity, &temp);

  float surface_temp = rtd_01.temperature(RNOMINAL, RREF);
  float underground_temp = rtd_02.temperature(RNOMINAL, RREF);

  snprintf(msg,
           sizeof(msg),
           "OUTDOOR_POE_UNIT_02: internal_temp=%.2f, internal_humidity=%.2f, surface_temp=%.2f, underground_temp=%.2f\n",
           temp.temperature,
           humidity.relative_humidity,
           surface_temp,
           underground_temp);

  Serial.print(msg);

  SendUdpPacket(msg);

  delay(60000);
}

void loop1() { delay(1000); }
