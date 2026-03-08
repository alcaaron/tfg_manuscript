from dataclasses import dataclass, field
import math
from lib.phy import LoRaPacket, LoRaParameters

@dataclass
class Node:
    node_id: int
    x: float
    y: float
    tx_period_ms: int = 1000  # duty cycle
    last_tx_time: float = 0.0
    received_packets: list[LoRaPacket] = field(default_factory=lambda: [])

    def distance_to(self, other: "Node") -> float:
        return math.sqrt((self.x - other.x)**2 + (self.y - other.y)**2)

    def create_packet(self, current_time: float, payload_bytes: int = 20):
        params = LoRaParameters(sf=7, bw=125, cr=1, tx_power_dbm=14, frequency_hz=868e6)
        return LoRaPacket(
            node_id=self.node_id,
            payload_bytes=payload_bytes,
            params=params,
            distance_m=0.0,  # updated in channel
            sender_id=self.node_id,
            timestamp=current_time
        )

    def receive(self, packet: LoRaPacket):
        distance = packet.distance_m
        if distance > 150:  # broadcast range limit
            return
        print(f"Node {self.node_id} received packet from {packet.sender_id} RSSI={packet.rssi:.1f} SNR={packet.snr:.1f}")
        if packet.collided or packet.lost:
            print(f"packet lost/collided: node {self.node_id}")
        else:
            self.received_packets.append(packet)