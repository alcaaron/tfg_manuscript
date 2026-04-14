#include <Arduino.h>
#include "LoRaDriver.h"

lib::lora::LoRaDriver lora;

TaskHandle_t rxTaskHandle;
TaskHandle_t processTaskHandle;

QueueHandle_t rxQueue;

#define RX_LORA_QUEUE_SIZE 10
#define POLLING_INTERVAL_MS 1000
#define RX_BUFFER_SIZE 100


void rxLoRaPacketTask(void *pvParameters)
{
  TickType_t xLastWakeTime;
  const TickType_t xFrequency = POLLING_INTERVAL_MS / portTICK_PERIOD_MS;
  xLastWakeTime = xTaskGetTickCount();
  for (;;)
  {
    static TickType_t lastWakeTime = xTaskGetTickCount();
    vTaskDelayUntil( &xLastWakeTime, xFrequency );
    Serial.println("Checking for packets every 1000 ms...");
    Serial.println();
    if (lora.checkPacketReceived())
    {
      Serial.println("Packet received!");
      uint8_t rx_buffer[RX_BUFFER_SIZE];
      int len = lora.readData(rx_buffer, sizeof(rx_buffer));

      if (len > sizeof(lib::packet::A3MESHHeader))
      {
        Serial.println("Received packet with valid header length");
        xQueueSend(rxQueue, &rx_buffer, portMAX_DELAY);
      }
      else
      {
        Serial.println("Received packet with invalid header length");
        Serial.println("Read length: " + String(len));
        Serial.println("Expected header length: " + String(sizeof(lib::packet::A3MESHHeader)));
      }
    }
  }
  vTaskDelete(NULL);
}

void processLoRaPacketTask(void *pvParameters)
{
  uint8_t rx_buffer[RX_BUFFER_SIZE];
  for (;;)
  {
    if (xQueueReceive(rxQueue, &rx_buffer, portMAX_DELAY) == pdTRUE)
    {
      Serial.println("Processing received packet...");

      // print raw content in hexa
      Serial.print("Raw content: ");
      int rx_packet_len = sizeof(rx_buffer)/sizeof(rx_buffer[0]);
      for (int i = 0; i < rx_packet_len; i++)
      {
        Serial.print(rx_buffer[i], HEX);
        Serial.print(" ");
      }
      Serial.println();

      lib::packet::A3MESHHeader *pktHeader = new lib::packet::A3MESHHeader();
      pktHeader->type = rx_buffer[0];
      pktHeader->version = rx_buffer[1];
      pktHeader->length = (rx_buffer[3] << 8) | rx_buffer[2];

      if (pktHeader->length > 0)
      {
        Serial.print("Received header: ");
        Serial.print("Type: ");
        Serial.print(pktHeader->type);
        Serial.print(", Version: ");
        Serial.print(pktHeader->version);
        Serial.print(", Length: ");
        Serial.println(pktHeader->length);
        Serial.println();
      }
      else
      {
        Serial.println("Error reading packet");
      }

      if (pktHeader->length > 0)
      {
        uint8_t payload[pktHeader->length];
        memcpy(payload, &rx_buffer[4], pktHeader->length);

        Serial.print("Payload: ");
        for (int i = 0; i < pktHeader->length; i++)
        {
          Serial.print(payload[i], HEX);
          Serial.print(" ");
        }
        Serial.println();

        lib::packet::A3MESHPacket_t payload_packet = {
            .pkt_id = ((uint16_t)payload[1] << 8) | payload[0],
            .node_id = ((uint16_t)payload[3] << 8) | payload[2],
            .src = ((uint16_t)payload[5] << 8) | payload[4],
            .dst = ((uint16_t)payload[7] << 8) | payload[6],
            .ttl = payload[8],
            .type = payload[9],
            .flags = payload[10],
            .payload_len = payload[11]};
        memcpy(payload_packet.payload, &payload[12], payload_packet.payload_len);

        Serial.print("Parsed payload packet: ");
        Serial.print("pkt_id: ");
        Serial.print(payload_packet.pkt_id);
        Serial.print(", node_id: ");
        Serial.print(payload_packet.node_id);
        Serial.print(", src: ");
        Serial.print(payload_packet.src);
        Serial.print(", dst: ");
        Serial.print(payload_packet.dst);
        Serial.print(", ttl: ");
        Serial.print(payload_packet.ttl);
        Serial.print(", type: ");
        Serial.print(payload_packet.type);
        Serial.print(", flags: ");
        Serial.print(payload_packet.flags);
        Serial.print(", payload_len: ");
        Serial.println(payload_packet.payload_len);

        Serial.print("Payload content: ");
        for (int i = 0; i < payload_packet.payload_len; i++)
        {
          Serial.print((char)payload_packet.payload[i]);
          Serial.print(" ");
        }
        Serial.println();
        Serial.println("---------------------------------------------------");
        Serial.println();
        Serial.println();
      }
    }
  }
  vTaskDelete(NULL);
}

bool setupA3MESH()
{
  if (!lora.begin(lib::lora::A3MESHConfig()))
  {
    return false;
  }

  lora.setPacketReceivedAction();
  xTaskCreate(rxLoRaPacketTask, "RX LoRa Packet Task", 4096, NULL, 1, NULL);
  xTaskCreate(processLoRaPacketTask, "Process LoRa Packet Task", 4096, NULL, 1, &processTaskHandle);
  rxQueue = xQueueCreate(RX_LORA_QUEUE_SIZE, sizeof(uint8_t) * RX_BUFFER_SIZE);

  if (!lora.startReceive())
  {
    return false;
  }
  return true;
}


void setup()
{
  Serial.begin(115200);
  Serial.println("initBoard");

  if (!setupA3MESH())
  {
    Serial.println("LoRa init failed");
    while (1)
      ;
  }
}

void loop()
{
}