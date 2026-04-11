#include <Arduino.h>
#include "LoRaDriver.h"

lib::lora::LoRaDriver lora;

#define IS_INITIATOR false   // nodo A (PING)= true, nodo B (PONG)= false

void sendMessage(const char* msg) {
  lora.sendRawPacket((uint8_t*)msg, strlen(msg));
  Serial.print("Sent: ");
  Serial.println(msg);
  lora.startReceive();
}

void setup() {
  Serial.begin(115200);

  bool ok = lora.begin(lib::lora::A3MESHConfig());
  if (!ok) {
    Serial.println("Failed to initialize LoRa");
    while (1);
  }

  lora.setDioActionsForReceivePacket();
  lora.startReceive();

  Serial.println("LoRa ready");

  if (IS_INITIATOR) {
    delay(1000);
    sendMessage("PING");
  }
}

void loop() {

  if (lora.available()) {

    uint8_t buffer[255];
    int len = lora.readData(buffer, sizeof(buffer));

    if (len > 0) {

      buffer[len] = '\0';

      Serial.print("Received: ");
      Serial.println((char*)buffer);

      if (strcmp((char*)buffer, "PING") == 0) {
        sendMessage("PONG");
      }
      else if (strcmp((char*)buffer, "PONG") == 0) {
        delay(500);
        sendMessage("PING");
      }
    }
  }
}