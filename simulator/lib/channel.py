from typing import List
from lib.node import Node
from lib.phy import LoRaPHY, LoRaPacket
import math

class LoRaChannel:
    """
    Shared wireless medium abstraction.

    Responsible for:
    - broadcasting packets
    - computing sender→receiver distance
    - invoking PHY evaluation
    - detecting packet collisions
    """
    def __init__(self, phy: LoRaPHY):
        self.phy = phy
        self.active_packets: List[LoRaPacket] = []

    def broadcast(self, sender_id: int, nodes: List[Node], packet: LoRaPacket):
        # deliver packet to all nodes except sender
        for node in nodes:
            if node.node_id == sender_id:
                continue
            # compute Euclidean distance between tx and rx
            packet.distance_m = math.sqrt((node.x - nodes[sender_id-1].x)**2 + (node.y - nodes[sender_id-1].y)**2)
            self.phy.evaluate_packet(packet)
            node.receive(packet)
        
        # store packet in channel to allow collision comparison
        self.active_packets.append(packet)
        
        # check collision with other active packets
        for p in self.active_packets:
            # if power difference is smaller than capture threshold, then both packets collide
            if p != packet and abs(packet.rssi - p.rssi) < self.phy.CAPTURE_THRESHOLD_DB:
                packet.collided = True
                p.collided = True