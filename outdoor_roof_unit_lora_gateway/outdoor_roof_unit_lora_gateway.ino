#include <SPI.h>
#include <RadioLib.h>

// EXACT SAME PINS as your working TX
#define LORA_SS     13
#define LORA_DIO1   16
#define LORA_RST    23
#define LORA_BUSY   18
#define LORA_ANT_SW 17  // RX stays HIGH

// SAME DECLARATION as working TX
Module radio(LORA_SS, LORA_DIO1, LORA_RST, LORA_BUSY, SPI1);
SX1262 lora(&radio);

// ISR flag
volatile bool receivedFlag = false;
void setFlag(void) {
  receivedFlag = true;
}

void setup() {
  Serial.begin(9600);
  delay(1000);

  // Antenna switch to RX (stays HIGH)
  pinMode(LORA_ANT_SW, OUTPUT);
  digitalWrite(LORA_ANT_SW, HIGH);

  // SPI1 setup (matches TX)
  SPI1.setRX(24);
  SPI1.setTX(15);
  SPI1.setSCK(14);
  SPI1.begin(false);

  // SAME INIT as TX
  int state = lora.begin(868.0);
  Serial.print("RX Init: ");
  Serial.println(state == RADIOLIB_ERR_NONE ? "OK" : String(state));

  // Sensitivity boost
  lora.setRxBoostedGainMode(true);

  // RX interrupt
  lora.setPacketReceivedAction(setFlag);
  state = lora.startReceive();
  Serial.print("RX Start: ");
  Serial.println(state == RADIOLIB_ERR_NONE ? "OK" : String(state));

  Serial.println("RX ready - 868 MHz HF");
  Serial.println("Antenna attached?");
}

void loop() {
  // Live RSSI every second (diagnostic)
  static unsigned long lastRssi = 0;
  if (millis() - lastRssi > 1000) {
    Serial.print("RSSI: ");
    Serial.print(lora.getRSSI());
    Serial.print(" dBm  SNR: ");
    Serial.print(lora.getSNR());
    Serial.println(" dB");
    lastRssi = millis();
  }

  // Packet received?
  if (!receivedFlag) return;
  receivedFlag = false;

  String str;
  int state = lora.readData(str);

  if (state == RADIOLIB_ERR_NONE) {
    Serial.print("✓ PACKET RX: ");
    Serial.println(str);
    Serial.print("   RSSI: "); Serial.print(lora.getRSSI()); Serial.println(" dBm");
    Serial.print("   SNR:  "); Serial.println(lora.getSNR()); 
  } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
    Serial.println("✗ CRC fail");
  } else {
    Serial.print("✗ Read fail: "); Serial.println(state);
  }

  // Restart RX
  lora.startReceive();
}
