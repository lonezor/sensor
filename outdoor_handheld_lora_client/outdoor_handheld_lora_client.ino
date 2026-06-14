#include <SPI.h>
#include <RadioLib.h>

#define LORA_SS     13
#define LORA_DIO1   16
#define LORA_RST    23
#define LORA_BUSY   18
#define LORA_ANT_SW 17

// CORRECT v7.5.1 declaration
Module radio(LORA_SS, LORA_DIO1, LORA_RST, LORA_BUSY, SPI1);
SX1262 lora(&radio);

void setup() {
  Serial.begin(9600);
  delay(1000);

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
  
  Serial.print("TX power: ");
  //Serial.println(lora.getOutputPower());
}

void loop() {
  Serial.println("TX...");
  digitalWrite(LORA_ANT_SW, LOW);
  lora.transmit("Hello!");
  digitalWrite(LORA_ANT_SW, HIGH);
  delay(3000);
}
