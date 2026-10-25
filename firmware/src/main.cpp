// ================
// === INCLUDES ===
// ================

#include <Arduino.h>
#include "A3MESHManager.hh"
#include "LoRaTasks.h"
#include "BLETasks.h"
#include "A3MESHManagerTasks.h"
#include "PassiveACK.h"
#include "CryptoManager.hh"
#include "Battery.hh"
#include "PowerManager.hh"
#include "OTAUpdater.hh"

// ======================
// === Global Objects ===
// ======================

lib::ble::BLEDriver ble;
lib::lora::LoRaDriver lora;
lib::mesh::A3MESHManager a3mesh;
lib::crypto::CryptoManager crypto;
lib::power::Battery battery;
lib::power::PowerManager power_mgr;

// Current LoRa radio parameters; updated by AT+LORA= and reported by AT+INFO?
// CR uses arduino-LoRa denominator convention: 5=4/5, 6=4/6, 7=4/7, 8=4/8.
// A3MESH_CR (config.h) = 1 is clamped to 5 by the library, so we start at 5.
static uint8_t  g_lora_sf = A3MESH_SF;
static uint16_t g_lora_bw = BANDWIDTH_250_KHZ / 1000;
static uint8_t  g_lora_cr = A3MESH_CR + 4;

// Parse a lowercase/uppercase hex string of exactly (out_len*2) chars into bytes.
static bool parse_hex_bytes(const char *hex, uint8_t *out, size_t out_len)
{
    for (size_t i = 0; i < out_len; i++) {
        auto h2n = [](char c) -> int8_t {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            return -1;
        };
        int8_t hi = h2n(hex[2 * i]), lo = h2n(hex[2 * i + 1]);
        if (hi < 0 || lo < 0) return false;
        out[i] = (uint8_t)((hi << 4) | lo);
    }
    return true;
}

// PassiveACK.h wrappers; implemented here so lower-level libs
// can call them without depending on the mesh include chain.
void mesh_register_pending_forward(uint16_t pkt_id, uint32_t src) {
    a3mesh.register_pending_forward(pkt_id, src);
}
void mesh_cancel_pending_forward(uint16_t pkt_id, uint32_t src) {
    a3mesh.cancel_pending_forward(pkt_id, src);
}
bool mesh_consume_pending_forward(uint16_t pkt_id, uint32_t src) {
    return a3mesh.consume_pending_forward(pkt_id, src);
}


// ======================
// ====== Queues ========
// ======================

QueueHandle_t xBLERxQueue;
QueueHandle_t xBLETxQueue;

QueueHandle_t xLoRaRxQueue;
QueueHandle_t xLoRaTxQueue;


// ===============================
// ===== BLE Tasks & Queues ======
// ===============================

bool init_BLE_queues()
{
  xBLETxQueue = xQueueCreate(10, sizeof(char[BLE_BUFFER_SIZE]));
  xBLERxQueue = xQueueCreate(10, sizeof(char[BLE_BUFFER_SIZE]));

  return xBLERxQueue != NULL && xBLETxQueue != NULL;
}

bool start_BLE_tasks()
{
  if (xTaskCreatePinnedToCore(vBLETxTask, "BLE TX Task", 4096, NULL, 1, NULL, 1) != pdPASS) return false;

  return true;
}


// ================================
// ===== LoRa Tasks & Queues ======
// ================================

bool initLoRaQueues()
{
  xLoRaRxQueue = xQueueCreate(10, sizeof(lib::lora::LoRaDriver::rx_LoRa_msg_t*));
  xLoRaTxQueue = xQueueCreate(10, sizeof(lib::lora::LoRaDriver::tx_LoRa_msg_t));

  return xLoRaRxQueue != NULL && xLoRaTxQueue != NULL;
}

bool startLoRaTasks()
{
  if(xTaskCreatePinnedToCore(vLoRaRxTask, "LoRa RX", 4096, NULL, 2, NULL, 0) != pdPASS) return false;

  if(xTaskCreatePinnedToCore(vLoRaTxTask, "LoRa TX", 4096, NULL, 1, NULL, 0) != pdPASS) return false;

  return true;
}

// =================================
// ===== Main Application Logic ====
// =================================

enum class MeshRxState
{
  k_rx_idle,

  k_rx_validate,
  k_rx_classify,

  k_rx_process_discovery,
  k_rx_process_data,
  k_rx_process_key,

  k_rx_local_delivery,
  k_rx_forward,

  k_rx_drop,
  k_rx_done
};

struct a3mesh_rx_ctx_t
{
  lib::lora::LoRaDriver::rx_LoRa_msg_t* rx_msg;

  lib::packet::A3MESH_header_t header;
  lib::packet::A3MESH_packet_t pkt;

  bool    should_forward;
  bool    is_for_me;
  uint8_t copies;   // 1 = first receipt, 2+ = duplicate; drives f_dupe in smart gossip
};

MeshRxState handleValidate(a3mesh_rx_ctx_t &ctx)
{
  int8_t ok = lib::packet::A3MESHPacket::deserialize_pkt(
      ctx.rx_msg->rx_data,
      ctx.rx_msg->len,
      ctx.header,
      ctx.pkt);

  if (ok < 0){
    MESH_LOG("[A3MESH] Failed to deserialize packet, e: %d\r\n", ok);
    return MeshRxState::k_rx_drop;
  }

  if (ctx.pkt.src == a3mesh.get_node_id())
    return MeshRxState::k_rx_drop;

  if (ctx.pkt.node_id == a3mesh.get_node_id())
    return MeshRxState::k_rx_drop;

  // A neighbor is forwarding this packet, then cancel our own pending forward if any.
  // Must run before get_copies so it fires even for packets already in our dup cache.
  a3mesh.cancel_pending_forward(ctx.pkt.pkt_id, ctx.pkt.src);

  if (ctx.pkt.ttl == 0)
    return MeshRxState::k_rx_drop;

  // Count copies: 1 = first receipt, 2+ = already seen.
  // Drop after GOSSIP_MAX_COPIES to bound loop potential while allowing f_dupe to act.
  ctx.copies = a3mesh.get_copies(ctx.pkt);
  if (ctx.copies > GOSSIP_MAX_COPIES)
    return MeshRxState::k_rx_drop;

  return MeshRxState::k_rx_classify;
}

MeshRxState handleClassify(a3mesh_rx_ctx_t &ctx)
{
  switch (ctx.header.type)
  {
  case lib::packet::k_discovery_type:
    return MeshRxState::k_rx_process_discovery;

  case lib::packet::k_data_plain:
  case lib::packet::k_data_encrypted:
    return MeshRxState::k_rx_process_data;

  case lib::packet::k_key_exchange:
    return MeshRxState::k_rx_process_key;

  default:
    return MeshRxState::k_rx_drop;
  }
}

MeshRxState handleDiscovery(a3mesh_rx_ctx_t &ctx)
{
  power_mgr.notify_activity();

  lib::mesh::Neighbor neighbor;

  neighbor.node_id = ctx.pkt.src;
  neighbor.rssi = ctx.rx_msg->rssi;
  neighbor.last_seen = millis();

  bool is_new = a3mesh.update_neighbor(neighbor);

  if (is_new)
  {
    char disc_buf[32];
    snprintf(disc_buf, sizeof(disc_buf), "+DISC=%08x\r\n", neighbor.node_id);
    xQueueSend(xBLETxQueue, disc_buf, 0);
  }

  return MeshRxState::k_rx_done;
}

MeshRxState handleData(a3mesh_rx_ctx_t &ctx)
{
  power_mgr.notify_activity();

  MESH_LOG("[A3MESH] dst=%08x me=%08x\r\n", ctx.pkt.dst, a3mesh.get_node_id());
  ctx.is_for_me =
      (ctx.pkt.dst == a3mesh.get_node_id()) ||
      (ctx.pkt.dst == BROADCAST_NODE_ID);

  if (ctx.is_for_me) {
    // Deliver locally only on first copy; subsequent copies are already handled.
    return (ctx.copies == 1) ? MeshRxState::k_rx_local_delivery : MeshRxState::k_rx_done;
  }

  MESH_LOG("[A3MESH] Not for me; But the payload is: %.*s\r\n", ctx.pkt.payload_len, ctx.pkt.payload);
  ctx.should_forward = a3mesh.should_forward(ctx.pkt, ctx.rx_msg->rssi, ctx.copies);

  if (ctx.should_forward)
    return MeshRxState::k_rx_forward;

  return MeshRxState::k_rx_drop;
}

MeshRxState handleKeyExchange(a3mesh_rx_ctx_t &ctx)
{
  ctx.is_for_me =
      (ctx.pkt.dst == a3mesh.get_node_id()) ||
      (ctx.pkt.dst == BROADCAST_NODE_ID);

  return (ctx.is_for_me) ? 
  MeshRxState::k_rx_local_delivery 
  : MeshRxState::k_rx_drop;
  
}

MeshRxState handleLocalDelivery(a3mesh_rx_ctx_t &ctx)
{
  MESH_LOG("[A3MESH] Delivering packet locally\r\n");
  char bleBuffer[BLE_BUFFER_SIZE] = {0};
  int len = 0;

  if (ctx.header.type == lib::packet::k_key_exchange)
  {
    len = snprintf(bleBuffer, sizeof(bleBuffer),
        "+KX=%08x,%08x,%.*s\r\n",
        ctx.pkt.src, ctx.pkt.node_id,
        ctx.pkt.payload_len, ctx.pkt.payload);
  }
  else if (ctx.header.type == lib::packet::k_data_encrypted)
  {
    uint8_t plain[A3MESH_MAX_PAYLOAD];
    lib::crypto::CryptoError cerr;
    uint32_t group_id = 0;

    if (ctx.pkt.type == lib::packet::k_enc_node2node)
    {
      cerr = crypto.decrypt_n2n(ctx.pkt.src, ctx.pkt, plain, sizeof(plain));
      if (cerr != lib::crypto::CryptoError::OK)
      {
        MESH_LOG("[CRYPTO] n2n decrypt failed: %d\r\n", (int)cerr);
        return MeshRxState::k_rx_drop;
      }
      len = snprintf(bleBuffer, sizeof(bleBuffer),
          "+RCVE2E=%08x,%08x,%.*s\r\n",
          ctx.pkt.src, ctx.pkt.node_id,
          ctx.pkt.payload_len, ctx.pkt.payload);
    }
    else if (ctx.pkt.type == lib::packet::k_enc_group)
    {
      cerr = crypto.decrypt_grp(ctx.pkt.src, ctx.pkt, plain, sizeof(plain), group_id);
      if (cerr != lib::crypto::CryptoError::OK)
      {
        MESH_LOG("[CRYPTO] grp decrypt failed: %d\r\n", (int)cerr);
        return MeshRxState::k_rx_drop;
      }
      len = snprintf(bleBuffer, sizeof(bleBuffer),
          "+RCVGRP=%08x,%08x,%08x,%.*s\r\n",
          group_id, ctx.pkt.src, ctx.pkt.node_id,
          ctx.pkt.payload_len, ctx.pkt.payload);
    }
    else if (ctx.pkt.type == lib::packet::k_enc_network)
    {
      cerr = crypto.decrypt_net(ctx.pkt.src, ctx.pkt, plain, sizeof(plain));
      if (cerr != lib::crypto::CryptoError::OK)
      {
        MESH_LOG("[CRYPTO] net decrypt failed: %d\r\n", (int)cerr);
        return MeshRxState::k_rx_drop;
      }
      // Deliver as a plain RCV so the app treats it transparently
      len = snprintf(bleBuffer, sizeof(bleBuffer),
          "+RCV=%08x,%08x,%08x,%.*s\r\n",
          ctx.pkt.src, ctx.pkt.node_id, ctx.pkt.dst,
          ctx.pkt.payload_len, ctx.pkt.payload);
    }
    else
    {
      return MeshRxState::k_rx_drop;
    }
  }
  else
  {
    len = snprintf(bleBuffer, sizeof(bleBuffer),
        "+RCV=%08x,%08x,%08x,%.*s\r\n",
        ctx.pkt.src, ctx.pkt.node_id, ctx.pkt.dst,
        ctx.pkt.payload_len, ctx.pkt.payload);
  }

  if (len <= 0)
    return MeshRxState::k_rx_drop;

  xQueueSend(xBLETxQueue, bleBuffer, 0);
  return MeshRxState::k_rx_done;
}

MeshRxState handleForward(a3mesh_rx_ctx_t &ctx)
{
  lib::lora::LoRaDriver::tx_LoRa_msg_t tx_msg = {};

  // Save before modifying the packet fields below
  const uint16_t orig_pkt_id = ctx.pkt.pkt_id;
  const uint32_t orig_src    = ctx.pkt.src;

  ctx.pkt.node_id = a3mesh.get_node_id();
  ctx.pkt.ttl--;
  ctx.pkt.flags = (a3mesh.is_neighbor(ctx.pkt.dst));

  tx_msg.len        = lib::packet::A3MESHPacket::serialize_header_pkt(ctx.header, ctx.pkt, tx_msg.tx_data);
  tx_msg.delay_ms   = (uint16_t)map(ctx.rx_msg->rssi, -120, -40, 20, 200);
  tx_msg.pkt_id     = orig_pkt_id;
  tx_msg.src        = orig_src;
  tx_msg.is_forward = true;

  MESH_LOG("[A3MESH] Forwarding pkt_id=%u, RSSI: %d, TX delay: %d ms\r\n",
           orig_pkt_id, ctx.rx_msg->rssi, tx_msg.delay_ms);

  // Register before queuing so the slot exists if a cancellation arrives during the delay.
  a3mesh.register_pending_forward(orig_pkt_id, orig_src);

  if (xQueueSend(xLoRaTxQueue, &tx_msg, 0) != pdTRUE) {
    MESH_LOG("[A3MESH] TX queue full, forward packet dropped\r\n");
    a3mesh.consume_pending_forward(orig_pkt_id, orig_src);  // free the slot
  }

  return MeshRxState::k_rx_done;
}

void LoRa2BLETask(void *pvParameters)
{
  lib::lora::LoRaDriver::rx_LoRa_msg_t* frame;

  for (;;)
  {
    if (xQueueReceive(xLoRaRxQueue, &frame, portMAX_DELAY) != pdTRUE)
      continue;

    a3mesh_rx_ctx_t ctx ;
    ctx.rx_msg = frame;

    MeshRxState state = MeshRxState::k_rx_validate;

    while (state != MeshRxState::k_rx_done)
    {
      switch (state)
      {

      case MeshRxState::k_rx_validate:
        state = handleValidate(ctx);
        break;

      case MeshRxState::k_rx_classify:
        state = handleClassify(ctx);
        break;

      case MeshRxState::k_rx_process_discovery:
        state = handleDiscovery(ctx);
        break;

      case MeshRxState::k_rx_process_data:
        state = handleData(ctx);
        break;

      case MeshRxState::k_rx_process_key:
        state = handleKeyExchange(ctx);
        break;

      case MeshRxState::k_rx_local_delivery:
        state = handleLocalDelivery(ctx);
        break;

      case MeshRxState::k_rx_forward:
        state = handleForward(ctx);
        break;

      case MeshRxState::k_rx_drop:
        state = MeshRxState::k_rx_done;
        break;

      default:
        state = MeshRxState::k_rx_done;
        break;
      }
    }
  }
}

void BLE2LoRaTask(void *pvParameters)
{
  char rx_buffer[BLE_BUFFER_SIZE];

  const uint32_t node_id = a3mesh.get_node_id();

  lib::packet::A3MESH_header_t header = {};
  memcpy(header.version, A3M_VERSION, sizeof(A3M_VERSION)); // size checked in A3MESHPacket.h

  lib::packet::A3MESH_packet_t pkt = {
      .src = node_id,
      .node_id = node_id,
  };

  lib::lora::LoRaDriver::tx_LoRa_msg_t tx_msg = {};

  for (;;)
  {
    if (xQueueReceive(xBLERxQueue, &rx_buffer, portMAX_DELAY) != pdTRUE)
      continue;

    // -------------------------
    // AT basic validation
    // -------------------------
    if (!BLE_MSG_STARTS_WITH(rx_buffer, "AT"))
    {
      xQueueSend(xBLETxQueue, "+ERR=1\r\n", 0);
      continue;
    }


    // -------------------------
    // AT command
    // Format: AT\r\n
    // Firmware responds with +OK\r\n
    // -------------------------
    if (strcmp(rx_buffer, "AT\r\n") == 0)
    {
      /*
        Expected send format: AT\r\n
      */
      xQueueSend(xBLETxQueue, "+OK\r\n", 0);
      continue;
    }


    // -------------------------
    // SEND command
    // Format: AT+SEND=<dst>,<payload>\r\n
    // Firmware sends the payload as a plain data packet, unencrypted.
    // -------------------------
    if (BLE_MSG_STARTS_WITH(rx_buffer, "AT+SEND="))
    {
      uint32_t dst = 0;

      char msg[A3MESH_MAX_PAYLOAD+1];

      const char *args = rx_buffer + 8;

      // memset(pkt.payload, 0, sizeof(pkt.payload));

      if (sscanf(args, 
                  "%08x,%" STR(A3MESH_MAX_PAYLOAD) "[^\r\n]", 
                  &dst, 
                  msg) != 2)
      {
        xQueueSend(xBLETxQueue, "+ERR=3\r\n", 0);
        continue;
      }

      // -------------------------
      // Packet fill (dynamic fields only)
      // -------------------------
      header.type = lib::packet::k_data_plain;
      pkt.type = lib::packet::k_enc_none;
      pkt.ttl = MAX_TTL;
      pkt.pkt_id = a3mesh.get_next_pkt_id();
      pkt.dst = dst;
      pkt.flags = (a3mesh.is_neighbor(dst)) ? 0x01 : 0x00;
      pkt.payload_len = (uint8_t)strlen(msg);

      memcpy(pkt.payload, msg, pkt.payload_len);

      tx_msg.len = lib::packet::A3MESHPacket::serialize_header_pkt(header, pkt, tx_msg.tx_data);

      if (xQueueSend(xLoRaTxQueue, &tx_msg, 0) == pdTRUE)
      {
        xQueueSend(xBLETxQueue, "+OK\r\n", 0);
      }
      else
      {
        xQueueSend(xBLETxQueue, "+ERR=4\r\n", 0);
      }
      continue;
    }


    // -------------------------
    // NEIGHBORS command
    // Format: AT+NEIGHBORS?\r\n
    // Firmware responds with a list of neighbors and their RSSI values and timestamp.
    // -------------------------
    if (strcmp(rx_buffer, "AT+NEIGHBORS?\r\n") == 0)
    {
      char response[BLE_BUFFER_SIZE];
      char neighbors_str[BLE_BUFFER_SIZE];

      lib::mesh::Neighbor neighbors_list[MAX_NEIGHBORS];
      const uint8_t count = a3mesh.get_neighbor_count();

      if (count == 0)
      {
        snprintf(response,
          sizeof(response),
          "+NEIGHBORS=0\r\n+OK\r\n");

        xQueueSend(xBLETxQueue, response, 0);
        continue;
      }

      a3mesh.get_neighbors(neighbors_list);

      int offset = 0;

      for (uint8_t i = 0; i < count; i++)
      {
        offset += snprintf(
            neighbors_str + offset,
            sizeof(neighbors_str) - offset,
            "+N:%08x,RSSI:%d,T:%lu;\r\n",
            neighbors_list[i].node_id,
            neighbors_list[i].rssi,
            neighbors_list[i].last_seen);

        if (offset >= sizeof(neighbors_str))
          break;
      }

      snprintf(response,
        sizeof(response),
        "+NEIGHBORS=%d\r\n%s\r\n+OK\r\n",
        count,
        neighbors_str);

      xQueueSend(xBLETxQueue, response, 0);
      continue;
    }


    // -------------------------
    // NODEID command
    // Format: AT+NODEID?\r\n
    // Firmware responds with the node's unique ID in hex format.
    // -------------------------
    if (strcmp(rx_buffer, "AT+NODEID?\r\n") == 0)
    {
      char response[BLE_BUFFER_SIZE];

      snprintf(response,
        sizeof(response),
        "+NODEID=%08x\r\n",
        node_id);

      xQueueSend(xBLETxQueue, response, 0);
      continue;
    }

    // -------------------------
    // SENDKX command
    // Format: AT+SENDKX=<dst_hex>,<payload>\r\n
    // Firmware sends the payload as a key exchange packet, unencrypted.
    // -------------------------
    if (BLE_MSG_STARTS_WITH(rx_buffer, "AT+SENDKX="))
    {
      uint32_t dst = 0;
      char msg[A3MESH_MAX_PAYLOAD + 1];
      const char *args = rx_buffer + 10;

      if (sscanf(args,
                  "%08x,%" STR(A3MESH_MAX_PAYLOAD) "[^\r\n]",
                  &dst, msg) != 2)
      {
        xQueueSend(xBLETxQueue, "+ERR=3\r\n", 0);
        continue;
      }

      header.type = lib::packet::k_key_exchange;
      pkt.type = lib::packet::k_enc_none;
      pkt.ttl = MIN_TTL;
      pkt.pkt_id = a3mesh.get_next_pkt_id();
      pkt.dst = dst;
      pkt.flags = (a3mesh.is_neighbor(dst)) ? DIRECT_UNICAST_FLAG : BROADCAST_FLAG;
      pkt.payload_len = (uint8_t)strlen(msg);

      memcpy(pkt.payload, msg, pkt.payload_len);

      tx_msg.len = lib::packet::A3MESHPacket::serialize_header_pkt(header, pkt, tx_msg.tx_data);

      if (xQueueSend(xLoRaTxQueue, &tx_msg, 0) == pdTRUE)
        xQueueSend(xBLETxQueue, "+OK\r\n", 0);
      else
        xQueueSend(xBLETxQueue, "+ERR=4\r\n", 0);
      continue;
    }


    // -------------------------
    // SENDE2E command
    // Format: AT+SENDE2E=<dst_hex>,<plaintext>\r\n
    // Firmware encrypts with the provisioned peer key for dst.
    // -------------------------
    if (BLE_MSG_STARTS_WITH(rx_buffer, "AT+SENDE2E="))
    {
      uint32_t dst = 0;
      char msg[A3MESH_MAX_PAYLOAD + 1];
      const char *args = rx_buffer + 11;

      if (sscanf(args,
                  "%08x,%" STR(A3MESH_MAX_PAYLOAD) "[^\r\n]",
                  &dst, msg) != 2)
      {
        xQueueSend(xBLETxQueue, "+ERR=3\r\n", 0);
        continue;
      }

      header.type = lib::packet::k_data_encrypted;
      pkt.type    = lib::packet::k_enc_node2node;
      pkt.ttl     = MAX_TTL;
      pkt.pkt_id  = a3mesh.get_next_pkt_id();
      pkt.dst     = dst;
      pkt.flags   = (a3mesh.is_neighbor(dst)) ? DIRECT_UNICAST_FLAG : BROADCAST_FLAG;

      lib::crypto::CryptoError cerr = crypto.encrypt_n2n(
          node_id, dst,
          (const uint8_t *)msg, strlen(msg),
          pkt);

      if (cerr != lib::crypto::CryptoError::OK)
      {
        xQueueSend(xBLETxQueue, "+ERR=5\r\n", 0); // +ERR=5 = crypto error / no key
        continue;
      }

      tx_msg.len = lib::packet::A3MESHPacket::serialize_header_pkt(header, pkt, tx_msg.tx_data);

      if (xQueueSend(xLoRaTxQueue, &tx_msg, 0) == pdTRUE)
        xQueueSend(xBLETxQueue, "+OK\r\n", 0);
      else
        xQueueSend(xBLETxQueue, "+ERR=4\r\n", 0);
      continue;
    }


    // -------------------------
    // SENDGRP command
    // Format: AT+SENDGRP=<group_id_hex>,<plaintext>\r\n
    // Firmware encrypts with the provisioned group key.
    // -------------------------
    if (BLE_MSG_STARTS_WITH(rx_buffer, "AT+SENDGRP="))
    {
      uint32_t group_id = 0;
      char msg[A3MESH_MAX_PAYLOAD + 1];
      const char *args = rx_buffer + 11;

      if (sscanf(args,
                  "%08x,%" STR(A3MESH_MAX_PAYLOAD) "[^\r\n]",
                  &group_id, msg) != 2)
      {
        xQueueSend(xBLETxQueue, "+ERR=3\r\n", 0);
        continue;
      }

      header.type = lib::packet::k_data_encrypted;
      pkt.type    = lib::packet::k_enc_group;
      pkt.ttl     = MAX_TTL;
      pkt.pkt_id  = a3mesh.get_next_pkt_id();
      pkt.dst     = BROADCAST_NODE_ID;
      pkt.flags   = BROADCAST_FLAG;

      lib::crypto::CryptoError cerr = crypto.encrypt_grp(
          node_id, group_id,
          (const uint8_t *)msg, strlen(msg),
          pkt);

      if (cerr != lib::crypto::CryptoError::OK)
      {
        xQueueSend(xBLETxQueue, "+ERR=5\r\n", 0);
        continue;
      }

      tx_msg.len = lib::packet::A3MESHPacket::serialize_header_pkt(header, pkt, tx_msg.tx_data);

      if (xQueueSend(xLoRaTxQueue, &tx_msg, 0) == pdTRUE)
        xQueueSend(xBLETxQueue, "+OK\r\n", 0);
      else
        xQueueSend(xBLETxQueue, "+ERR=4\r\n", 0);
      continue;
    }


    // -------------------------
    // SENDNET command
    // Format: AT+SENDNET=<dst_hex>,<plaintext>\r\n
    // Firmware encrypts with the shared network key.
    // -------------------------
    if (BLE_MSG_STARTS_WITH(rx_buffer, "AT+SENDNET="))
    {
      uint32_t dst = 0;
      char msg[A3MESH_MAX_PAYLOAD + 1];
      const char *args = rx_buffer + 11;

      if (sscanf(args,
                  "%08x,%" STR(A3MESH_MAX_PAYLOAD) "[^\r\n]",
                  &dst, msg) != 2)
      {
        xQueueSend(xBLETxQueue, "+ERR=3\r\n", 0);
        continue;
      }

      header.type = lib::packet::k_data_encrypted;
      pkt.type    = lib::packet::k_enc_network;
      pkt.ttl     = MAX_TTL;
      pkt.pkt_id  = a3mesh.get_next_pkt_id();
      pkt.dst     = dst;
      pkt.flags   = (a3mesh.is_neighbor(dst)) ? DIRECT_UNICAST_FLAG : BROADCAST_FLAG;

      lib::crypto::CryptoError cerr = crypto.encrypt_net(
          node_id,
          (const uint8_t *)msg, strlen(msg),
          pkt);

      if (cerr != lib::crypto::CryptoError::OK)
      {
        xQueueSend(xBLETxQueue, "+ERR=5\r\n", 0);
        continue;
      }

      tx_msg.len = lib::packet::A3MESHPacket::serialize_header_pkt(header, pkt, tx_msg.tx_data);

      if (xQueueSend(xLoRaTxQueue, &tx_msg, 0) == pdTRUE)
        xQueueSend(xBLETxQueue, "+OK\r\n", 0);
      else
        xQueueSend(xBLETxQueue, "+ERR=4\r\n", 0);
      continue;
    }


    // -------------------------
    // Key management commands
    // AT+SETNETKEY=<64hex>              --> set 32-byte shared network key
    // AT+SETPEERKEY=<08hex>,<64hex>     --> set node-to-node key for peer
    // AT+SETGRPKEY=<08hex>,<64hex>      --> set group key for group_id
    // AT+CLRKEYS                        --> wipe all stored keys
    // -------------------------
    if (BLE_MSG_STARTS_WITH(rx_buffer, "AT+SETNETKEY="))
    {
      const char *hex = rx_buffer + 13;
      uint8_t key[lib::crypto::CRYPTO_KEY_SIZE];
      if (!parse_hex_bytes(hex, key, sizeof(key)))
      {
        xQueueSend(xBLETxQueue, "+ERR=3\r\n", 0);
        continue;
      }
      crypto.keys.set_net_key(key);
      xQueueSend(xBLETxQueue, "+OK\r\n", 0);
      continue;
    }

    if (BLE_MSG_STARTS_WITH(rx_buffer, "AT+SETPEERKEY="))
    {
      const char *args = rx_buffer + 14;
      uint32_t peer_id = 0;
      char hex[lib::crypto::CRYPTO_KEY_SIZE * 2 + 2];
      if (sscanf(args, "%08x,%64s", &peer_id, hex) != 2)
      {
        xQueueSend(xBLETxQueue, "+ERR=3\r\n", 0);
        continue;
      }
      uint8_t key[lib::crypto::CRYPTO_KEY_SIZE];
      if (!parse_hex_bytes(hex, key, sizeof(key)))
      {
        xQueueSend(xBLETxQueue, "+ERR=3\r\n", 0);
        continue;
      }
      if (!crypto.keys.set_peer_key(peer_id, key))
      {
        xQueueSend(xBLETxQueue, "+ERR=6\r\n", 0); // key table full
        continue;
      }
      xQueueSend(xBLETxQueue, "+OK\r\n", 0);
      continue;
    }

    if (BLE_MSG_STARTS_WITH(rx_buffer, "AT+SETGRPKEY="))
    {
      const char *args = rx_buffer + 13;
      uint32_t group_id = 0;
      char hex[lib::crypto::CRYPTO_KEY_SIZE * 2 + 2];
      if (sscanf(args, "%08x,%64s", &group_id, hex) != 2)
      {
        xQueueSend(xBLETxQueue, "+ERR=3\r\n", 0);
        continue;
      }
      uint8_t key[lib::crypto::CRYPTO_KEY_SIZE];
      if (!parse_hex_bytes(hex, key, sizeof(key)))
      {
        xQueueSend(xBLETxQueue, "+ERR=3\r\n", 0);
        continue;
      }
      if (!crypto.keys.set_group_key(group_id, key))
      {
        xQueueSend(xBLETxQueue, "+ERR=6\r\n", 0);
        continue;
      }
      xQueueSend(xBLETxQueue, "+OK\r\n", 0);
      continue;
    }

    if (strcmp(rx_buffer, "AT+CLRKEYS\r\n") == 0)
    {
      crypto.keys.clear_all();
      crypto.replay.reset_all();
      xQueueSend(xBLETxQueue, "+OK\r\n", 0);
      continue;
    }


    // -------------------------
    // INFO command
    // Format: AT+INFO?\r\n
    // Returns: NODEID, FW version, battery %, uptime, LoRa params, neighbor count
    // -------------------------
    if (strcmp(rx_buffer, "AT+INFO?\r\n") == 0)
    {
      // --- Battery percentage (LUT-based via Battery class) ---
      int bat_pct = (int)battery.get_battery_percentage();

      // --- Uptime ---
      uint32_t uptime_s = millis() / 1000UL;
      uint32_t h = uptime_s / 3600;
      uint32_t m = (uptime_s % 3600) / 60;
      uint32_t s = uptime_s % 60;

      // --- LoRa params ---
      uint32_t freq_mhz = (uint32_t)(EU868_FREQUENCY / 1000000UL);

      char response[BLE_BUFFER_SIZE];
      snprintf(response, sizeof(response),
          "+NODEID=%08x\r\n"
          "+FWVER=%s\r\n"
          "+BATT=%d\r\n"
          "+UPTIME=%02u:%02u:%02u\r\n"
          "+FREQ=%u\r\n"
          "+SF=SF%d\r\n"
          "+BW=%d kHz\r\n"
          "+CR=4/%d\r\n"
          "+NEIGHBORS=%d\r\n"
          "+OK\r\n",
          node_id,
          A3M_VERSION,
          bat_pct,
          h, m, s,
          freq_mhz,
          (int)g_lora_sf,
          (int)g_lora_bw,
          (int)g_lora_cr,
          (int)a3mesh.get_neighbor_count());

      xQueueSend(xBLETxQueue, response, 0);
      continue;
    }


    // -------------------------
    // LORA config
    // AT+LORA=<SF>,<BW>,<CR>   --> reconfigure radio (SF: 7-12, BW: 125/250/500 kHz, CR: 5-8)
    // AT+LORA?                 --> query current config
    // -------------------------
    if (strcmp(rx_buffer, "AT+LORA?\r\n") == 0)
    {
      char response[48];
      snprintf(response, sizeof(response),
               "+SF=SF%d\r\n+BW=%d kHz\r\n+CR=4/%d\r\n+OK\r\n",
               (int)g_lora_sf, (int)g_lora_bw, (int)g_lora_cr);
      xQueueSend(xBLETxQueue, response, 0);
      continue;
    }

    if (BLE_MSG_STARTS_WITH(rx_buffer, "AT+LORA="))
    {
      int sf = 0, bw = 0, cr = 0;
      if (sscanf(rx_buffer + 8, "%d,%d,%d", &sf, &bw, &cr) != 3 ||
          sf < 7 || sf > 12 ||
          (bw != 125 && bw != 250 && bw != 500) ||
          cr < 5 || cr > 8)
      {
        xQueueSend(xBLETxQueue, "+ERR=3\r\n", 0);
        continue;
      }
      LoRa.idle();
      LoRa.setSpreadingFactor((uint8_t)sf);
      LoRa.setSignalBandwidth((float)bw * 1000.0f);
      LoRa.setCodingRate4((uint8_t)cr);
      LoRa.receive();
      g_lora_sf = (uint8_t)sf;
      g_lora_bw = (uint16_t)bw;
      g_lora_cr = (uint8_t)cr;
      xQueueSend(xBLETxQueue, "+OK\r\n", 0);
      continue;
    }

    // -------------------------
    // Fallback
    // -------------------------
    xQueueSend(xBLETxQueue, "+ERR=2\r\n", 0);
  }
}

void vSendDiscoveryTask(void *pvParameters)
{
  const uint32_t node_id = a3mesh.get_node_id();
  // Create discovery packet msg
  lib::packet::A3MESH_header_t header = {};
  memcpy(header.version, A3M_VERSION, sizeof(A3M_VERSION)); // size checked in A3MESHPacket.h
  header.type = lib::packet::a3mesh_msg_type_t::k_discovery_type;

  lib::packet::A3MESH_packet_t pkt = {
      .pkt_id = a3mesh.get_next_pkt_id(),
      .src = node_id,
      .node_id = node_id,
      .dst = BROADCAST_NODE_ID, 
      .ttl = MIN_TTL,
      .type = lib::packet::a3mesh_enc_type_t::k_enc_none,
      .flags = BROADCAST_FLAG,
      .payload_len = 4
  };

  memcpy(pkt.payload, "DISC", pkt.payload_len);
  
  lib::lora::LoRaDriver::tx_LoRa_msg_t discovery_msg = {};
  discovery_msg.len = lib::packet::A3MESHPacket::serialize_header_pkt(
      header,
      pkt,
      discovery_msg.tx_data);

  for (;;)
  {
    // NOTE: pkt_id is patched directly in the serialized buffer, as the first
    // field (little endian) right after the A3MESH_header_t. Must be updated if
    // the serialization format changes.
    uint16_t pkt_id = a3mesh.get_next_pkt_id();
    discovery_msg.tx_data[lib::packet::HEADER_SIZE]     = pkt_id & 0xFF;
    discovery_msg.tx_data[lib::packet::HEADER_SIZE + 1] = (pkt_id >> 8) & 0xFF;
    xQueueSend(xLoRaTxQueue, &discovery_msg, 0);
    vTaskDelay(pdMS_TO_TICKS(DISCOVERY_INTERVAL_MS) + (pdMS_TO_TICKS(rand() % 10000)));
  }
  vTaskDelete(NULL);
}

bool setup_A3MESH()
{
  // --------- Init SPI ---------
  SPI.begin(SCK_GPIO, MISO_GPIO, MOSI_GPIO, NSS_GPIO);


  // --------- Init LoRa Driver ---------
  if (!lora.begin(lib::lora::A3MESHConfig())) return false;
  MESH_LOG("[LORA] init success!\r\n");

  if (!initLoRaQueues()) return false;
  MESH_LOG("[LORA] queues initialized!\r\n");

  if (!startLoRaTasks()) return false;
  MESH_LOG("[LORA] tasks started!\r\n");


  // --------- Init BLE Driver ---------
  if (!ble.setup()) return false;
  MESH_LOG("[BLE] init success!\r\n");

  if (!init_BLE_queues()) return false;
  MESH_LOG("[BLE] queues initialized!\r\n");

  if (!start_BLE_tasks()) return false;
  MESH_LOG("[BLE] tasks started!\r\n");


  // --------- Init Core Tasks ---------
  if (xTaskCreatePinnedToCore(BLE2LoRaTask, "BLE2LoRaTask", 8192, NULL, 1, NULL, 1) != pdPASS) return false;
  MESH_LOG("[SYS] BLE2LORA started!\r\n");

  if (xTaskCreatePinnedToCore(LoRa2BLETask, "LoRa2BLETask", 8192, NULL, 1, NULL, 1) != pdPASS) return false;
  MESH_LOG("[SYS] LORA2BLE started!\r\n");


  // --------- Init A3MESH Manager ---------
  if (xTaskCreatePinnedToCore(vSendDiscoveryTask, "DiscoveryTask", 4096, NULL, 1, NULL, 0) != pdPASS) return false;
  MESH_LOG("[MESH] Discovery Task Initialized!\r\n");

  if (xTaskCreatePinnedToCore(vNeighborCheckTask, "NeighborCheckTask", 4096, NULL, 1, NULL, 0) != pdPASS) return false;
  MESH_LOG("[MESH] Neighbor Check Task Initialized!\r\n");

  // --------- Init Power Manager ---------
  if (!power_mgr.start_task()) return false;
  MESH_LOG("[PWR] Power Manager Task Initialized!\r\n");

  return true;
}

void setup()
{
  Serial.begin(BAUD_RATE);
  MESH_LOG("[SYS] initBoard\r\n");

  delay(1000);
  switch (esp_reset_reason())
  {
  case ESP_RST_BROWNOUT:
    MESH_LOG("[SYS] power down detected\r\n");
    break;
  case ESP_RST_POWERON:
    MESH_LOG("[SYS] power on detected\r\n");
    break;
  case ESP_RST_WDT:
    MESH_LOG("[SYS] watch dog timer\r\n");
    break;
  case ESP_RST_TASK_WDT:
    MESH_LOG("[SYS] task WDT\r\n");
    break;
  case ESP_RST_PANIC:
    MESH_LOG("[SYS] self reset\r\n");
    break;
  default:
    break;
  }

#if OTA_ENABLED
  // --------- OTA firmware check ---------
  // Runs before setup_A3MESH() so WiFi never coexists with BLE/LoRa: it is
  // switched off before returning.  On a successful update the node reboots
  // inside check_and_update(); otherwise the boot continues normally.
  lib::ota::OTAUpdater ota;
  ota.check_and_update();
#endif

  if (!setup_A3MESH())
  {
    MESH_LOG("[SYS] Setup failed\r\n");
    while (1)
      ;
  }
}

void loop()
{
}