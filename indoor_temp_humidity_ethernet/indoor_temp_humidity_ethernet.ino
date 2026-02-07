
#include <Wire.h>

#include "CH9120.h"
#include "Adafruit_SHTC3.h"

UCHAR CH9120_LOCAL_IP[4] = {172, 17, 0, 21};
UCHAR CH9120_GATEWAY[4] = {172, 17, 0, 1};
UCHAR CH9120_SUBNET_MASK[4] = {255, 255, 255, 0};
UCHAR CH9120_REMOTE_IP[4] = {172, 17, 0, 20};
UWORD CH9120_LOCAL_PORT = 21;
UWORD CH9120_REMOTE_PORT = 20;

static Adafruit_SHTC3 shtc3 = Adafruit_SHTC3();
static char msg[1024];

void setup() {
  Serial.begin(115200);

  // Configure the I2C pins for GP0 (SDA) and GP1 (SCL)
  Wire.setSDA(0);
  Wire.setSCL(1);
  Wire.begin();

  delay(2000);

  if (!shtc3.begin(&Wire)) {
    while (1) { 
      Serial.println("Couldn't find SHTC3 sensor!");
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

void loop() {
  sensors_event_t humidity, temp;
  
  shtc3.getEvent(&humidity, &temp);

  snprintf(msg,
           sizeof(msg),
           "TEMP_HUMIDITY_SENSOR_ETH_UNIT_01: temperature=%.2f, humidity=%.2f\n",
           temp.temperature,
           humidity.relative_humidity);

  Serial.print(msg);

  SendUdpPacket(msg);

  delay(60000);
}

void loop1() {
  delay(1000);
}
