import math
from dataclasses import dataclass

# Sensitivity table SF/BW
SENSITIVITY_TABLE = {
    7:  {125: -126.5, 250: -124.25, 500: -120.75},
    8:  {125: -127.25, 250: -126.75, 500: -124.0},
    9:  {125: -131.25, 250: -128.25, 500: -127.5},
    10: {125: -132.75, 250: -130.25, 500: -128.75},
    11: {125: -134.5,  250: -132.75, 500: -128.75},
    12: {125: -133.25, 250: -132.25, 500: -132.25},
}

@dataclass
class LoRaParameters:
    sf: int                 # spreading factor
    bw: int                 # bandwidth in kHz   
    cr: int                 # coding rate (1-4, where 1 means 4/5)
    tx_power_dbm: float     # transmit power in dBm
    frequency_hz: float     # Carrier frequency

@dataclass
class LoRaPacket:
    node_id: int
    payload_bytes: int
    params: LoRaParameters
    distance_m: float
    sender_id: int
    timestamp: float = 0.0

    rssi: float = 0.0
    snr: float = 0.0
    airtime: float = 0.0
    collided: bool = False
    lost: bool = False

class PropagationModel:
    def __init__(self, gamma: float = 2.08, d0: float = 40.0, Lpld0: float = 127.41, GL: float = 0.0):
        self.gamma = gamma
        self.d0 = d0
        self.Lpld0 = Lpld0
        self.GL = GL

    def path_loss(self, distance_m: float) -> float:
        """
        Compute path-loss in dB using log-distance path loss model
        """
        return self.Lpld0 + 10 * self.gamma * math.log10(distance_m / self.d0)

    def received_power(self, tx_power_dbm: float, distance_m: float) -> float:
        """
        Estimate received power in dBm at given distance
        """
        return tx_power_dbm - self.GL - self.path_loss(distance_m)

class LoRaPHY:
    """
    Abstraction of the LoRa physical layer.

    Responsible for:
    - RSSI calculation
    - sensitivity threshold checks
    - airtime estimation
    """
    CAPTURE_THRESHOLD_DB = 6.0

    def __init__(self, propagation: PropagationModel):
        self.propagation = propagation

    def sensitivity(self, sf: int, bw: int) -> float:
        return SENSITIVITY_TABLE[sf][bw]

    def airtime(self, sf: int, cr: int, pl: int, bw: int) -> float:
        """
        Compute airtime using the standard formula
        """
        h = 0
        de = 0
        Npream = 8
        if bw == 125 and sf in [11,12]:
            de = 1
        if sf == 6:
            h = 1
        Tsym = (2.0**sf)/bw
        Tpream = (Npream+4.25)*Tsym
        payloadSymbNB = 8 + max(math.ceil((8.0*pl -4.0*sf+28+16-20*h)/(4.0*(sf-2*de)))*(cr+4),0)
        return Tpream + payloadSymbNB*Tsym

    def evaluate_packet(self, packet: LoRaPacket):
        """
        Evaluate the received packet and update its RSSI, SNR, airtime, and loss status
        """
        packet.rssi = self.propagation.received_power(packet.params.tx_power_dbm, packet.distance_m)
        packet.airtime = self.airtime(packet.params.sf, packet.params.cr, packet.payload_bytes, packet.params.bw)
        sensitivity = self.sensitivity(packet.params.sf, packet.params.bw)
        if packet.rssi < sensitivity:
            packet.lost = True
        packet.snr = packet.rssi - sensitivity