# A3MESH Node Firmware

Firmware for the A3MESH LoRa mesh node, developed as part of the
Bachelor's Thesis *A3MESH: A Decentralised LoRa Mesh Platform for
Off-Grid Smartphone Messaging*. The firmware runs on an ESP32-WROOM-32UE
paired with a Semtech SX1276 (RFM95W) transceiver and is written in
C++17 on top of the Arduino framework and FreeRTOS. It is built with
PlatformIO.

## Repository Layout

```
tfg_firmware/
└── firmware/
    ├── platformio.ini            Board, framework and library dependencies
    ├── include/
    │   ├── config.h              Compile-time constants (SF, BW, TTL, keys, ...)
    │   └── PassiveACK.h          Passive-ACK forward-cancel interface
    ├── src/
    │   └── main.cpp              FreeRTOS task wiring and AT-command parser
    ├── lib/
    │   ├── ble/                  NUS GATT server; BLE TX/RX tasks
    │   ├── crypto/               AES-256-GCM layer (AESGCM, CryptoManager,
    │   │                         CryptoKeys, NonceManager, ReplayProtection)
    │   ├── lora/                 LoRa radio driver (RadioLib wrapper)
    │   ├── mesh/                 A3MESHManager: neighbour table, Smart Gossip,
    │   │                         duplicate cache, neighbour-check task
    │   ├── packet/               A3MESH packet serialisation / deserialisation
    │   └── power/                Battery LUT and adaptive CPU-frequency manager
    ├── test/                     Native unit tests
    └── custom.csv                Custom NVS partition table
```

Appendix D of the manuscript summarises this layout; Appendix A gives
the complete AT-command reference exposed over BLE.

## Building and Flashing

The project has no external toolchain dependencies beyond
[PlatformIO](https://platformio.org/). Once the extension or CLI is
installed, the following commands are sufficient:

```bash
# From the firmware/ directory
pio run                       # compile
pio run --target upload       # flash over USB
pio device monitor            # open a serial monitor (115200 baud)
```

The default upload port is auto-detected; on Linux, ensure the user
belongs to the `dialout` group. The board exposes a USB-CDC serial
port that is used only for firmware logs — the operational data path
to the smartphone is BLE, not USB.

## Configuration

All build-time parameters live in `include/config.h`. A developer
introducing changes typically only needs to touch this file and the
`lib/mesh/` module. Notable groups of constants are:

- **Radio profile** — spreading factor, bandwidth, coding rate,
  frequency, and transmit power. The defaults match the LONG_FAST
  preset used throughout the thesis (SF11, BW 250 kHz, CR 4/5,
  868 MHz, 27 dBm).
- **Mesh parameters** — hop limit, retransmission timeouts, and the
  Smart Gossip constants (`alpha`, `RSSI_ref`, `beta`, `gamma`,
  `p_floor`) obtained by differential-evolution optimisation.
- **Cryptographic keys** — placeholders for the group-level pre-shared
  key. Peer keys are provisioned at runtime through the AT interface
  and stored in the ESP32 NVS partition defined by `custom.csv`.

## Runtime Architecture

The firmware is structured as a set of cooperating FreeRTOS tasks
communicating through thread-safe queues. The main tasks are:

- `vLoRaRxTask` and `vLoRaTxTask` — drive the SX1276 radio through the
  RadioLib abstraction, feeding received frames into the mesh manager
  and dequeuing outbound frames.
- `vBLE2LoRaTask` — parses AT commands received over the Nordic UART
  Service and dispatches the resulting actions (send message,
  provision key, query state, etc.). The complete command set is
  documented in Appendix A of the manuscript.
- `vPowerTask` — scales the CPU frequency between 80 MHz and 240 MHz
  according to activity level, minimising idle-mode current draw.

Any incoming packet is authenticated and decrypted by the `crypto/`
layer before being surfaced to the mesh manager; replay protection is
enforced by a monotonic per-source nonce window.

## Extending the Firmware

- **Adding a new AT command** — declare the parser branch in
  `src/main.cpp` inside `vBLE2LoRaTask` and, where relevant, expose a
  helper in the appropriate `lib/` module. Return values must always
  terminate with `+OK\r\n` or `+ERR=n\r\n` as specified in
  Appendix A.
- **Modifying the routing logic** — Smart Gossip is implemented in
  `lib/mesh/A3MESHManager`. Changes to the probability model should be
  reflected in the simulator (`MeshtasticatorMeshAlgoComp`) to keep
  the reference implementation and the on-device implementation
  aligned.
- **Porting to a new radio module** — replace the RadioLib driver
  instance in `lib/lora/` and adjust the SPI pin assignments in
  `platformio.ini` and `include/config.h`.

## Reference Documentation

The reader is referred to Chapter 3, Section 5 of the accompanying thesis
manuscript for a detailed treatment of the firmware architecture,
Section 4 for the hardware it targets, and Section 3 for the design
and validation of the Smart Gossip forwarding algorithm.
