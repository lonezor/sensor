
#include <Wire.h>

#include "Adafruit_SHTC3.h"
#include "CH9120.h"

#include <SPI.h>
#include <Wire.h>

#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>
#include <SensirionI2cSht3x.h>

#define BME_SCK 2  // GPIO 2 - SPI Clock
#define BME_MISO 4 // GPIO 4 - SDO (Data out from sensor)
#define BME_MOSI 3 // GPIO 3 - SDI (Data in to sensor)
#define BME_CS 5   // GPIO 5 - Chip Select

#define SEALEVELPRESSURE_HPA (1013.25)

// Software SPI
static Adafruit_BME280 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK);

static UCHAR CH9120_LOCAL_IP[4] = {172, 16, 0, 100};
static UCHAR CH9120_GATEWAY[4] = {172, 16, 0, 1};
static UCHAR CH9120_SUBNET_MASK[4] = {255, 255, 252, 0};
static UCHAR CH9120_REMOTE_IP[4] = {172, 16, 0, 200};
static UWORD CH9120_LOCAL_PORT = 100;
static UWORD CH9120_REMOTE_PORT = 200;

static char msg[1024];
static SensirionI2cSht3x sht3x;

void setup() {
  Serial.begin(115200);

  // Configure the I2C pins for GP0 (SDA) and GP1 (SCL)
  Wire.setSDA(0);
  Wire.setSCL(1);
  Wire.begin();

  // Configure the I2C pins for GP6 (SDA) and GP7 (SCL)
  Wire1.setSDA(6);
  Wire1.setSCL(7);
  Wire1.begin();

  delay(2000);

  if (!bme.begin()) {
    while (1) {
      Serial.println("Could not find BME280 sensor!");
      Serial.println("Check wiring: VIN->3.3V, GND->GND, SCK->GP2, SDI->GP3, "
                     "SDO->GP4, CS->GP5");
      delay(1);
    }
  }

  sht3x.begin(Wire1, 0x44); // or 0x45 if ADDR/A0 pulled high

  CH9120_init(CH9120_LOCAL_IP, CH9120_GATEWAY, CH9120_SUBNET_MASK,
              CH9120_REMOTE_IP, CH9120_LOCAL_PORT, CH9120_REMOTE_PORT);

  memset(msg, 0, sizeof(msg));
}

void setup1() {
  // Currently, there is no need to use this core
  delay(1000);
}

void loop() {
  uint16_t error = 0;
  float outdoor_temp = 0;
  float outdoor_humidity = 0;
  char error_msg[64];
  memset(error_msg, 0, sizeof(error_msg));

  error = sht3x.measureSingleShot(REPEATABILITY_HIGH, // repeatability
                                  false, // disable clock stretching
                                  outdoor_temp, outdoor_humidity);
  if (error) {
    errorToString(error, error_msg, sizeof(error_msg));
    Serial.print("Error: ");
    Serial.println(error_msg);
  }

  snprintf(msg, sizeof(msg),
           "OUTDOOR_POE_UNIT_01: internal_temp=%.2f °C, internal_humidity=%.2f "
           "%, barometric_pressure %.2f hPa, outdoor_temp=%.2f, "
           "outdoor_humidity=%.2f\n",
           bme.readTemperature(), bme.readHumidity(),
           bme.readPressure() / 100.0F, outdoor_temp, outdoor_humidity);

  Serial.print(msg);

  SendUdpPacket(msg);

  delay(60000);
}

void loop1() { delay(1000); }
