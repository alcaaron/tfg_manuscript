#pragma once
#include <stdint.h>

// Thin wrappers over A3MESHManager's passive-ACK registry.
// Implemented in main.cpp, where a3mesh is instantiated, so that
// lower-level libraries (lora) can call them without depending on the
// mesh library include chain (which pulls in BLE headers).
void mesh_register_pending_forward(uint16_t pkt_id, uint32_t src);
void mesh_cancel_pending_forward(uint16_t pkt_id, uint32_t src);
bool mesh_consume_pending_forward(uint16_t pkt_id, uint32_t src);
