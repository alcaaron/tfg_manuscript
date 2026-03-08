import time
from lib.phy import LoRaPHY, PropagationModel
from lib.channel import LoRaChannel
from lib.node import Node

prop = PropagationModel()
phy = LoRaPHY(prop)
channel = LoRaChannel(phy)

nodes = [
    Node(1, 0, 0, tx_period_ms=1000),
    Node(2, 50, 0, tx_period_ms=1500),
    Node(3, 200, 0, tx_period_ms=2000),
]

SIMTIME_MS = 5000
dt_ms = 500
current_time = 0

while current_time < SIMTIME_MS:
    for node in nodes:
        # transmission triggered when duty cycle interval has elapsed
        if current_time - node.last_tx_time >= node.tx_period_ms:
            packet = node.create_packet(current_time)
            channel.broadcast(node.node_id, nodes, packet)

            node.last_tx_time = current_time
    current_time += dt_ms
    time.sleep(0.1)