#include <Arduino.h>
#include "LoRaDriver.h"

lib::lora::LoRaDriver lora;

bool setupA3MESH()
{
  return lora.begin(lib::lora::A3MESHConfig());
}

void setup()
{
  Serial.begin(115200);

  bool ok = setupA3MESH();
  if (!ok)
  {
    Serial.println("Failed to initialize LoRa");
    while (1)
      ;
  }

  lora.setDioActionsForReceivePacket();
  lora.startReceive();

  Serial.println("LoRa ready");
}

void loop()
{

  if (lora.available())
  {

    uint8_t rx_buffer[100];
    int len = lora.readData(rx_buffer, sizeof(rx_buffer));

    // print raw content in hexa
    Serial.print("Raw content: ");
    for (int i = 0; i < len; i++)
    {
      Serial.print(rx_buffer[i], HEX);
      Serial.print(" ");
    }
    Serial.println();

    lib::packet::A3MESHHeader *pktHeader = new lib::packet::A3MESHHeader();
    pktHeader->type = rx_buffer[0];
    pktHeader->version = rx_buffer[1];
    pktHeader->length = (rx_buffer[3] << 8) | rx_buffer[2];

    if (len > 0)
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