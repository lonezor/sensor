
#include <Wire.h>
#include "CH9120.h"

UCHAR CH9120_LOCAL_IP[4] = {192, 168, 202, 5};
UCHAR CH9120_GATEWAY[4] = {192, 168, 202, 1};
UCHAR CH9120_SUBNET_MASK[4] = {255, 255, 255, 0};
UCHAR CH9120_REMOTE_IP[4] = {192, 168, 202, 1};
UWORD CH9120_LOCAL_PORT = 5;
UWORD CH9120_REMOTE_PORT = 5;

static uint32_t motion_sensor_ts = 0;
static bool enabled = false;
static uint32_t iterations = 0;

void setup() {
  Serial.begin(115200);

  pinMode(0, INPUT_PULLUP);
  pinMode(1, INPUT);
  pinMode(2, OUTPUT);

  CH9120_init(CH9120_LOCAL_IP, CH9120_GATEWAY, CH9120_SUBNET_MASK,
              CH9120_REMOTE_IP, CH9120_LOCAL_PORT, CH9120_REMOTE_PORT);
}

void setup1() {
  // Currently, there is no need to use this core
 delay(10);
}

void loop() {
  int running = digitalRead(0);
  int motion = digitalRead(1);

  // Motion sensor activated
  if (running == LOW) {
    // sensor value is only the trigger point. The LED covers enabled flag
    if (motion == HIGH) {
       enabled = true;
       motion_sensor_ts = millis();
    }
  } else {
    enabled = false;
  }

  if (!enabled) {
    motion_sensor_ts = millis();
  }

  uint32_t elapsed = (uint32_t)millis() - motion_sensor_ts;
  uint32_t threshold = 10 * 60 * 1000;
   if (elapsed > threshold) {
        enabled = false;
        
    }

  if (enabled) {
    digitalWrite(2, HIGH);
  } else {
    digitalWrite(2, LOW);
  }

  char msg[1024];
  int len = snprintf(msg, sizeof(msg), "MOTION_SENSOR_ETH_UNIT_02: motion=%d, enabled=%d, elapsed=%d, threshold=%d\n",
    motion, enabled, elapsed, threshold);
  
  Serial.print(msg);

  if (iterations % 5 == 0) {
    SendUdpPacket(msg);
  }

  delay(50);

  iterations++;

}



void loop1() {
  delay(1000);
}
