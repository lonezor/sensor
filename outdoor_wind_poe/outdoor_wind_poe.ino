#include <Wire.h>

#include "CH9120.h"
#include "Adafruit_ADS1X15.h"
#include "Adafruit_SHTC3.h"

static UCHAR CH9120_LOCAL_IP[4] = {172, 16, 0, 102};
static UCHAR CH9120_GATEWAY[4] = {172, 16, 0, 1};
static UCHAR CH9120_SUBNET_MASK[4] = {255, 255, 252, 0};
static UCHAR CH9120_REMOTE_IP[4] = {172, 16, 0, 200};
static UWORD CH9120_LOCAL_PORT = 102;
static UWORD CH9120_REMOTE_PORT = 200;

static Adafruit_ADS1115 ads;
static Adafruit_SHTC3 shtc3 = Adafruit_SHTC3();

static char msg[1024];

void setup() {
  Serial.begin(115200);

  // Configure the I2C pins for GP0 (SDA) and GP1 (SCL)
  Wire.setSDA(0);
  Wire.setSCL(1);
  Wire.begin();

  // Configure the I2C pins for GP2 (SDA) and GP3 (SCL)
  Wire1.setSDA(2);
  Wire1.setSCL(3);
  Wire1.begin();

  if (!ads.begin()) {
    Serial.println("ADS1115 not found");
    while (1) delay(10);
  }

  ads.setGain(GAIN_ONE); // +/-4.096V

  if (!shtc3.begin(&Wire1)) {
    while (1) { 
      Serial.println("SHTC3 not found");
      delay(500);
    }
  }

  CH9120_init(CH9120_LOCAL_IP, CH9120_GATEWAY, CH9120_SUBNET_MASK,
              CH9120_REMOTE_IP, CH9120_LOCAL_PORT, CH9120_REMOTE_PORT);

  memset(msg, 0, sizeof(msg));
}

void setup1() {
  // Currently, there is no need to use this core
  delay(1000);
}

static float mapClamped(float v, float inMin, float inMax, float outMin, float outMax) {
  if (v <= inMin) return outMin;
  if (v >= inMax) return outMax;
  return outMin + (v - inMin) * (outMax - outMin) / (inMax - inMin);
}

void loop() {

  int16_t diff01 = ads.readADC_Differential_0_1();
  int16_t diff23 = ads.readADC_Differential_2_3();

  float v01 = ads.computeVolts(diff01);
  float v23 = ads.computeVolts(diff23);

  float windSpeed = mapClamped(v01, 1.0, 5.0, 0.0, 30.0);
  float windDir   = mapClamped(v23, 1.0, 5.0, 0.0, 360.0);

  // Observation: <0.2 is actually no wind
  if (windSpeed <0.2) {
    windSpeed = 0;
  }

  sensors_event_t humidity, temp;
  
  shtc3.getEvent(&humidity, &temp);

  snprintf(msg,
           sizeof(msg),
           "OUTDOOR_POE_UNIT_03: internal_temp=%.2f, internal_humidity=%.2f, wind_speed=%.2f m/s, wind_direction=%.2f degrees\n",
           temp.temperature,
           humidity.relative_humidity,
           windSpeed,
           windDir);

  Serial.print(msg);

  SendUdpPacket(msg);

  delay(500);
}

void loop1() { delay(1000); }
