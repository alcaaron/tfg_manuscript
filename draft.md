# TECHONOLOGIES IN SMARTPHONES

start at 19:45

## Links

[Medium](https://medium.com/coding-nexus/how-offline-mesh-messaging-works-inside-the-next-gen-of-communication-3187c2df995d)

[Kaspersky](https://www.kaspersky.com/blog/mesh-messengers/54192/)

[Findlight](https://www.findlight.net/blog/wireless-communication-protocols/)

[LoRa wikipedia](https://en.wikipedia.org/wiki/LoRa)

[IEEE article](https://ieeexplore-ieee-org.recursos.biblioteca.upc.edu/document/4460126)

[LoRaWan](https://lora-alliance.org/about-lorawan/)

[LoRa semtech](https://www.semtech.com/products/wireless-rf)

[IoT Protocols](https://www.jisem-journal.com/index.php/journal)

[NFC forum](https://nfc-forum.org/learn/nfc-technology/)

[IEEE Xplore BLE](https://ieeexplore-ieee-org.recursos.biblioteca.upc.edu/document/8419192)

[Bluetooth](https://www.bluetooth.com/about-us/who-we-are/)

[UWB and table](https://arxiv.org/pdf/2307.13066)

[Infrared](https://ieeexplore-ieee-org.recursos.biblioteca.upc.edu/stamp/stamp.jsp?tp=&arnumber=554222)



## Terms to study

P2P, Mesh, Ad-hoc

## Challenges

1. Range
2. Perfomance due to # of users
3. Battery
4. Cybersecurity (msg encryption)
5. Open source

## Brainstorming

- Use Lo-Ra for wide range
- Easy-to-use: integrate it with a flexible board with the phone
- Energy consumption:
  - Depend on phone battery
  - Context (movement, load, signal...)
- Dynamic adaption
  - when to transmit or not
  - prioritize critical messages
  - when use mesh or not (wi-fi or no wi-fi access)
- Take advantage of GPS and other phone sensors

### FW
- MCU LoRa
  - PHY + MAC
  - timestamps
  - messages buffer
- Smartphone
  - routing
  - priroization
  - energy decision making
  - mesh logic

### Interesting features
Opportunistic net hybrid cellular and mesh
- IF phone detects movement THEN increase duty
- IF low battery THEN reduce TX
- IF no cell signal THEN use mesh, activate LoRa
- IF cell signal THEN not use mesh, spread data through net

#### Architecture
PHONE
- controls energy
- decide TX
- sensors
- net logic

PCB (LoRa)
- RF
- antenna
- MCU or co-processor
---

## List of technologies
### Wireless Communication Protocols

- **Bluetooth LE & Mesh**: Allows devices to connect directly within ~100 meters. Mesh networks enable devices to pass messages along like a relay, extending the range across a crowd.

- **Wi-Fi Direct (Ad-hoc Mode)**: Uses the phone's Wi-Fi radio to talk directly to another device without needing a router or internet connection, offering higher speeds than Bluetooth.

- **NFC (Near Field Communication)**: Facilitates very close-range communication (usually a few centimeters) for immediate pairing or small data transfers.

- **UWB (Ultra Wideband)**: Provides high-speed, short-range communication, surpassing 100 meters, which is ideal for direct, fast, offline transfers.

- **USB Tethering**: Physical connection for transferring data or sharing network connectivity.

## Interesting APPs
- BitChat
- Briar
- Bridgefy
- Signal Offline Messenger
- White Mouse

## Near Field Communication (NFC)
Near Field Communication (NFC) is a key player among wireless communication protocols, designed for close-range interactions between devices. Operating at a frequency of 13.56 MHz, NFC allows for data exchange when devices are within a few centimeters of each other. It builds on the existing RFID (Radio Frequency Identification) technology, but with enhanced security and versatility.

How It Works: NFC uses magnetic field induction to enable communication between an active device (initiator) and a passive device (target). When the initiator generates a magnetic field, it induces a current in the target, powering it and facilitating data transfer. This process is quick and requires no pairing or manual configuration, making NFC highly user-friendly. The typical data transfer rate of NFC ranges from 106 to 424 Kbps, suitable for simple, quick exchanges of information.

### Applications
- Contactless Payments: NFC is widely adopted in mobile payment systems like Apple Pay, Google Wallet, and Samsung Pay, allowing users to make secure transactions with a simple tap of their smartphone or NFC-enabled card. According to Statista, the global transaction value of NFC mobile payments is projected to reach $15 trillion by 2024.
- Access Control: NFC is used in security systems for access control, providing secure entry to buildings, offices, and other restricted areas. Over 1 billion NFC-enabled access cards are estimated to be in use globally.
- Data Exchange: NFC enables quick data transfer between devices, such as sharing contacts, photos, or documents by bringing two devices close together. Approximately 2 billion smartphones worldwide are equipped with NFC technology.
- Public Transport: Many public transportation systems use NFC for ticketing, allowing passengers to tap their cards or smartphones to access services. In cities like London and Tokyo, millions of daily commuters use NFC-enabled cards and devices for seamless travel.

### Limitations
- Range: NFC’s effective range is limited to a few centimeters, which, while enhancing security, restricts its applications to very close interactions. This range is typically 4 cm or less.
- Speed: Data transfer rates for NFC are relatively low compared to other wireless communication protocols like Bluetooth (up to 3 Mbps) or WiFi (up to several Gbps), making it unsuitable for transferring large files.
- Power Requirements: Passive NFC devices rely on the initiator’s magnetic field for power, which can limit their functionality and range. This dependency on an external power source can also affect the device’s ability to perform complex tasks.
- Interference: NFC can be susceptible to interference from metal objects and other electronic devices, potentially affecting performance. Studies indicate that environmental factors can reduce NFC efficiency by up to 30%.

## Bluetooth

Operates in the 2.4 Ghz ISM (Industrial, Scientific and Medical) band.
Range: Up to 100m, depends on the device class and environment.

### How it works
Bluetooth uses frequency-hopping spread spectrum (FHSS) technology, which divides the frequency band into multiple channels and rapidly switches between them to minimize interference and maximize data integrity. Bluetooth devices are classified into three power classes:

- Class 1: Up to 100 meters range with 100 mW power.
- Class 2: Up to 10 meters range with 2.5 mW power (most common in mobile devices).
- Class 3: Up to 1 meter range with 1 mW power.

Bluetooth’s data transfer rates have evolved over time, with Bluetooth 5.0 supporting speeds up to 2 Mbps and the latest Bluetooth 5.2 offering even more advanced features like LE Audio for enhanced audio performance.

### Limitations
- Interference: Bluetooth shares the 2.4 GHz band with other devices like WiFi and microwave ovens, which can cause interference and affect performance. Studies show that co-channel interference can degrade Bluetooth throughput by up to 50%.
- Security: While Bluetooth includes security measures like pairing and encryption, it has been vulnerable to attacks such as Bluejacking, Bluesnarfing, and BlueBorne. Enhanced security protocols in Bluetooth 5.2 address many of these issues, but older devices remain susceptible.
- Range: The effective range of Bluetooth is relatively short compared to other wireless communication protocols like WiFi. Although Class 1 devices can reach up to 100 meters, most consumer devices (Class 2) are limited to about 10 meters.
- Power Consumption: Despite improvements with Bluetooth Low Energy (BLE), traditional Bluetooth connections can still consume significant power, impacting battery life for portable devices. BLE reduces power consumption by up to 50-100 times compared to classic Bluetooth, making it suitable for battery-operated IoT devices.


## Radio Frequency (RF)

Range: 3 kHz to 300 Ghz

### How it works
RF communication involves the transmission of electromagnetic waves through the air. Devices equipped with RF transmitters generate these waves, which are then received by RF receivers tuned to the same frequency. The data is modulated onto the carrier wave at the transmitter and demodulated at the receiver. RF signals can propagate through space, walls, and other obstacles, making them highly versatile for various communication needs.

RF communication is categorized into different frequency bands, each with specific characteristics:

- Low Frequency (LF): 30 kHz – 300 kHz, used for long-range applications like AM radio.
- High Frequency (HF): 3 MHz – 30 MHz, used for shortwave radio and some communication systems.
- Very High Frequency (VHF): 30 MHz – 300 MHz, used for FM radio, TV broadcasts, and marine communications.
Ultra High Frequency (UHF): 300 MHz – 3 GHz, used for TV broadcasts, mobile phones, WiFi, and Bluetooth.
- Microwave: 3 GHz – 30 GHz, used for satellite communication, radar, and certain WiFi frequencies.
- Millimeter Wave: 30 GHz – 300 GHz, used for advanced applications like 5G technology and high-frequency radar.

### Applications
- WiFi: RF is the underlying technology for WiFi, enabling wireless internet access in homes, offices, and public spaces. As of 2023, there are over 22 billion WiFi devices in use globally.
- Cellular Networks: RF powers cellular communication, from 2G to 5G, facilitating mobile phone and data services. The global mobile subscription count exceeds 8 billion.
- Remote Controls: RF is used in remote controls for TVs, garage doors, and other consumer electronics. RF remote controls operate over longer distances and do not require line-of-sight, unlike infrared (IR) remotes.
- Radio and Television Broadcasting: RF is essential for AM/FM radio and TV broadcasts, reaching millions of households worldwide.
- IoT Devices: RF technology, particularly in the UHF and microwave bands, is critical for connecting IoT devices, including smart home systems and industrial sensors. The number of IoT devices is projected to reach 75 billion by 2025.
- RFID: Radio Frequency Identification (RFID) uses RF signals to automatically identify and track tags attached to objects. It is widely used in logistics, retail, and access control.

### Limitations
- Interference: RF signals are susceptible to interference from other electronic devices, physical obstructions, and environmental factors. This can degrade signal quality and reliability. Studies indicate that RF interference can reduce network performance by up to 40%.
- Regulation: RF spectrum allocation and usage are heavily regulated by governments to prevent interference and ensure efficient use of the spectrum. This can limit the availability of certain frequencies for specific applications.
- Security: RF communications can be intercepted or jammed, posing security risks. Advanced encryption and secure protocols are necessary to protect sensitive data transmitted over RF.
- Range vs. Power: Higher frequency RF signals typically have shorter ranges and require more power, which can impact the design and deployment of wireless systems. For example, millimeter-wave 5G networks require dense deployment of small cells to maintain coverage.


## Wi-Fi
NOT GONNA BE USED HERE

## Zigbee
Zigbee is a wireless communication protocol designed for low-power, low-data-rate applications, often used in home automation, industrial control, and IoT (Internet of Things) devices. Operating on the IEEE 802.15.4 standard, Zigbee typically functions in the 2.4 GHz frequency band, though it can also operate in the 868 MHz and 915 MHz bands in certain regions.

### How it works
Zigbee employs a mesh network topology, where each device (or node) can communicate with any other node within its range, creating a robust and self-healing network. This mesh architecture enhances network reliability and coverage, as data can hop between nodes to reach its destination, bypassing obstacles and extending the effective range of the network. Zigbee networks can support thousands of nodes, making them highly scalable.

### Applications
- Home Automation: Zigbee is widely used in smart home devices, such as smart lights, thermostats, door locks, and security systems. According to a Zigbee Alliance report, over 500 million Zigbee-enabled devices were in use as of 2023.
- Industrial Control: Zigbee is employed in industrial environments for monitoring and controlling processes, machinery, and equipment, contributing to increased efficiency and safety.
- Smart Metering: Utilities use Zigbee for smart meters that communicate energy usage data, enabling more efficient energy management and billing.
- Health and Fitness Devices: Zigbee is used in medical monitoring devices and fitness trackers, providing reliable, low-power connectivity for health data transmission.
- Retail and Logistics: Zigbee tags and sensors are used in inventory management, asset tracking, and logistics to improve supply chain visibility and efficiency.

### Limitations
- Data Rate: Zigbee supports low data rates, typically up to 250 kbps, which limits its use to applications that do not require high bandwidth. This makes it unsuitable for streaming video or transferring large files.
- Range: The range of Zigbee devices is relatively short, usually around 10 to 100 meters, depending on the environment and specific implementation. While the mesh network topology mitigates this limitation, it can still be a constraint in certain applications.
- Interference: Operating in the crowded 2.4 GHz band, Zigbee can experience interference from other wireless technologies like WiFi and Bluetooth. Proper network planning and channel management are necessary to minimize this issue.
- Compatibility: While the Zigbee standard ensures interoperability, different manufacturers’ implementations can sometimes lead to compatibility issues, requiring careful selection of devices to ensure seamless integration.

Zigbee’s low power consumption, scalability, and robust mesh networking capabilities make it a popular choice for various IoT and automation applications. Its ability to support a large number of devices with minimal power requirements is particularly advantageous for battery-operated sensors and devices. As of 2023, the global Zigbee market was valued at approximately $4.5 billion and is expected to grow at a CAGR of 9% from 2024 to 2030, driven by the expanding IoT ecosystem and increasing demand for smart home and industrial automation solutions.


## Z-Wave
Z-Wave is a wireless communication protocol primarily used for home automation. It operates in the sub-GHz frequency range (around 908.42 MHz in the US and 868.42 MHz in Europe), which helps to reduce interference with other wireless technologies that commonly operate in the 2.4 GHz band. Z-Wave is known for its reliability, low power consumption, and secure communication, making it a popular choice for smart home devices.

### How it works
Z-Wave utilizes a mesh network topology, similar to Zigbee, where each device (or node) can communicate with other nodes within its range, forming a network where signals can hop between nodes. This extends the network’s range and enhances its robustness, as data can be routed through multiple paths to reach its destination. Z-Wave networks can support up to 232 devices, making them suitable for comprehensive home automation systems.

### Applications
- Home Automation: Z-Wave is widely used in smart home devices, such as lighting controls, thermostats, door locks, security systems, and sensors. As of 2023, there are over 100 million Z-Wave certified devices worldwide.
- Security Systems: Z-Wave enables the integration of various security devices like motion detectors, door/window sensors, and smart locks, providing a cohesive and responsive security network.
- Energy Management: Smart plugs and energy monitors using Z-Wave help homeowners track and manage energy consumption, contributing to energy efficiency and cost savings.
- Health and Wellness: Z-Wave is used in health monitoring devices, such as fall detectors and medication dispensers, to assist in elderly care and health management.
- Industrial Applications: Z-Wave is also used in some industrial applications for monitoring and control, although its primary focus remains on residential use.

### Limitations
- Data Rate: Z-Wave supports lower data rates compared to other wireless protocols, typically around 100 kbps. This limits its use to applications that require low bandwidth, such as sensor data and control commands.
- Range: The effective range of Z-Wave devices is about 30 meters indoors, which can be extended through the mesh network but still limits coverage compared to protocols like WiFi. Physical obstructions like walls and floors can also affect the range.
- Interference: Although Z-Wave operates in the sub-GHz band to avoid interference with 2.4 GHz devices, it can still be affected by other devices operating in the same frequency range, such as cordless phones and some garage door openers.
- Compatibility: While Z-Wave ensures interoperability among certified devices, there can be compatibility issues with devices from different manufacturers or older Z-Wave versions, requiring careful device selection and network management.

Z-Wave’s focus on home automation, coupled with its reliable, low-power operation, has made it a leading choice for smart home ecosystems. Its secure and efficient communication protocol ensures that home automation devices work seamlessly together, providing enhanced convenience and security for homeowners. As of 2023, the global Z-Wave market was valued at approximately $2.8 billion and is expected to grow at a CAGR of 10% from 2024 to 2030, driven by the increasing adoption of smart home technologies and the expanding range of Z-Wave certified products.

## Infrared (IR)
Infrared (IR) communication is a wireless technology that uses infrared light waves to transmit data. It is commonly used for short-range communication between devices in a line-of-sight configuration. Infrared light, with wavelengths longer than visible light but shorter than radio waves, is invisible to the human eye but can be detected by IR sensors.

### How it works
IR communication involves transmitting data via infrared light, typically using an IR LED (Light Emitting Diode) as the transmitter and a photodiode as the receiver. The data is encoded into the light signal, which is then sent to the receiver where it is decoded back into its original form. The most common IR communication standard is the Infrared Data Association (IrDA) protocol, which supports data transfer rates from 9600 bps to 4 Mbps.

### Applications
- Remote Controls: IR is widely used in remote controls for TVs, air conditioners, DVD players, and other consumer electronics. The IR remote control market has a penetration rate of over 90% in households worldwide.
- Short-Range Communication: IR is used for short-range data transfer between devices, such as between a laptop and a printer or between two smartphones. This application has largely been replaced by Bluetooth and WiFi Direct in recent years.
- Proximity Sensors: IR is used in proximity sensors for detecting objects or measuring distances, common in automatic doors, robotics, and touchless faucets.
- Medical Devices: IR technology is used in medical equipment such as pulse oximeters and heart rate monitors to measure vital signs through non-invasive methods.
- Optical Communication: IR is used in optical communication systems, such as fiber optics, where IR light is transmitted through optical fibers to carry data over long distances with minimal signal loss.

### Limitations
- Line of Sight: IR communication requires a clear line of sight between the transmitter and receiver, as infrared light cannot penetrate solid objects. This limits its usability in environments with obstacles.
- Range: The effective range of IR communication is relatively short, typically limited to a few meters. This is suitable for remote controls and short-range data transfer but not for longer distances.
- Interference: IR communication can be affected by ambient light sources, such as sunlight and fluorescent lights, which can interfere with the signal and reduce reliability. Proper shielding and modulation techniques are necessary to mitigate this issue.
- Data Rate: Although IR can support reasonable data rates, it is generally slower compared to other wireless communication protocols like WiFi and Bluetooth. The maximum data rate for IrDA is 4 Mbps, which is sufficient for simple data transfers but not for high-bandwidth applications.

Despite these limitations, IR remains a reliable and cost-effective solution for many applications, particularly in consumer electronics and sensing technologies. Its simplicity and effectiveness in line-of-sight communication make it an enduring choice for remote controls and other short-range applications. As of 2023, the global market for IR remote controls and IR-based sensors was valued at approximately $7 billion, with a projected CAGR of 5% from 2024 to 2030, driven by ongoing demand in consumer electronics and emerging applications in automation and medical devices.

## Ultra-Wideband (UWB)
Ultra-Wideband (UWB) is a wireless communication protocol known for its ability to transmit data over a wide range of frequencies with very low power. Unlike traditional narrowband communication systems, UWB uses a broad spectrum (typically from 3.1 to 10.6 GHz) to send short pulses of data, allowing for precise distance measurement and high data rates over short distances.

### How it works
UWB transmits data by emitting short-duration pulses across a wide frequency spectrum. These pulses have a very low power density, which helps to minimize interference with other wireless technologies. The wide bandwidth allows UWB to achieve high data rates and excellent spatial resolution, making it ideal for applications requiring precise location tracking and high-speed data transfer. UWB can measure the time it takes for a signal to travel between devices, providing accurate distance and position information with centimeter-level precision.

### Applications
- Precise Location Tracking: UWB is widely used for indoor positioning systems, offering accurate tracking of objects and people within buildings. This is beneficial in environments like warehouses, hospitals, and smart homes. As of 2023, UWB is used in over 100 million devices for indoor positioning.
- Contactless Access Control: UWB enables secure and precise access control systems, such as keyless entry for cars and buildings. It allows for hands-free entry with high accuracy, enhancing user convenience and security.
- Consumer Electronics: UWB is integrated into smartphones and smartwatches for features like precise device-to-device communication, spatial awareness, and enhanced wireless connectivity. Apple, Samsung, and other manufacturers have adopted UWB in their latest devices.
- Industrial Applications: UWB is used in industrial environments for asset tracking, worker safety, and automation. It enables real-time location systems (RTLS) that improve operational efficiency and safety.
- Healthcare: UWB is used in healthcare settings for patient tracking, equipment management, and monitoring. It helps in accurately locating medical equipment and personnel in hospitals.

### Limitations
- Range: UWB is optimized for short-range communication, typically up to 10 meters. While this is suitable for many applications, it limits UWB’s use in scenarios requiring longer-distance communication.
- Interference: Although UWB operates with low power to minimize interference, it can still be affected by other wireless technologies operating in the same frequency range. Proper design and filtering are necessary to ensure reliable performance.
- Adoption: While UWB is gaining traction, its adoption is still in the early stages compared to more established technologies like WiFi and Bluetooth. This can limit the availability of UWB-enabled devices and infrastructure.
- Cost: Implementing UWB technology can be more expensive than other wireless communication protocols due to the need for specialized hardware and precise calibration.

UWB’s unique capabilities make it an invaluable tool for applications requiring high precision and low power consumption. Its ability to provide accurate location data and high-speed communication over short distances sets it apart from other wireless protocols. As of 2023, the global UWB market was valued at approximately $1.5 billion, with a projected CAGR of 20% from 2024 to 2030. This growth is driven by increasing demand for precise location tracking, secure access control, and advanced consumer electronics features.

## LI-Fi (Light Fidelity)

is a wireless communication technology that uses light to transmit data. Unlike traditional WiFi, which uses radio waves, Li-Fi leverages visible light, ultraviolet, and infrared light from LEDs (Light Emitting Diodes) to enable high-speed data transfer. This emerging technology offers several advantages, including higher data rates, enhanced security, and reduced electromagnetic interference.

### How it works
Li-Fi operates by modulating the intensity of light emitted from an LED light bulb at extremely high speeds, imperceptible to the human eye. These rapid changes in light intensity encode binary data (0s and 1s), which is then received by a photodetector and converted back into an electronic signal. Li-Fi can support data transfer rates exceeding 10 Gbps, far surpassing typical WiFi speeds. Because Li-Fi relies on light, it requires a direct line of sight or light reflection within a room to maintain communication.


### Applications
- High-Speed Internet Access: Li-Fi can provide ultra-fast internet connectivity in homes, offices, and public spaces. Its high data transfer rates make it ideal for streaming high-definition video, online gaming, and other bandwidth-intensive applications.
- Secure Communications: Li-Fi offers enhanced security since light cannot penetrate walls, reducing the risk of eavesdropping and unauthorized access. This makes it suitable for secure communications in military, corporate, and financial settings.
- Industrial Automation: Li-Fi can be used in industrial environments where radio frequency interference is a concern, such as in factories with sensitive electronic equipment. It provides reliable and interference-free communication for industrial automation systems.
- Healthcare: Li-Fi is beneficial in healthcare settings where RF interference can affect medical equipment. It enables high-speed data transfer in hospitals and medical facilities without the risk of interfering with critical devices.
- Smart Lighting: Li-Fi can be integrated into smart lighting systems, enabling data communication through existing lighting infrastructure. This allows for dual-purpose lighting and communication networks in smart buildings and cities.

### Limitations
- Line of Sight: Li-Fi requires a direct line of sight between the transmitter and receiver, as light cannot pass through walls or opaque objects. This limits its range and necessitates multiple light sources to ensure coverage throughout a space.
- Interference from Ambient Light: Li-Fi performance can be affected by ambient light sources, such as sunlight and fluorescent lights, which can interfere with the signal. Proper modulation techniques and shielding are required to mitigate this issue.
- Range: The effective range of Li-Fi is limited to the coverage area of the light source, typically within a single room. This contrasts with WiFi, which can cover larger areas through walls and other obstructions.
- Infrastructure: Deploying Li-Fi requires significant changes to existing infrastructure, including the installation of LED light bulbs and compatible receivers. This can be cost-prohibitive and challenging to implement on a large scale.

Despite these limitations, Li-Fi’s potential for high-speed, secure, and interference-free communication makes it an exciting area of development. Its ability to integrate with existing lighting infrastructure and provide dual-purpose functionality is particularly appealing for smart buildings and urban environments. As of 2023, the global Li-Fi market was valued at approximately $1.2 billion, with a projected CAGR of 45% from 2024 to 2030, driven by increasing demand for high-speed wireless communication and the growing adoption of smart lighting systems. If you are interested to learn more on LiFi, check out our blog post on Li-Fi Technology.


---

# Mesh Algorithm design

Ad-Hoc Networks: Infrastructure-less, decentralized nodes, dynamic topology, self-organizing.
[Ad-Hoc](https://www.highfrequencyelectronics.com/Mar07/HFE0307_Tutorial.pdf)

Peer-to-peer: is the general concept of a networking model can we wired or not and also can be permanent or temporary, but ad-hoc is always temporary.

Mesh: Focuses on routing, where nodes actively relay data for other nodes to ensure connectivity across a wider area.

Network topologies:
1. Point to point
2. Bus
3. Ring
4. Star
5. Tree
6. **Mesh**
7. Hybrid


OSI model:


1. App: API to the phone
2. Presentation: Serialization, codification, cyphering, etc.
3. Session: 
4. Transport: ACK...
5. Network: Routing, Multi-hop, TTL, Node discovery, Forwarding, Duplicated surpression
6. Link: Framing, M.A.C, CRC...
7. Physical: LoRa modulation, Freq. 868 MHz, TX power, SF, BW, CR, Airtime; Check limitations


## Analyze LoRa limitations

Interesting pages:
[RadioHead](https://www.airspayce.com/mikem/arduino/RadioHead/examples.html)
[SX127x](https://www.mouser.com/pdfdocs/semtech-lora-modem-design.pdf?srsltid=AfmBOooXCxh_QLmI7r4JOWug3IKO390HMACHS_HrxPlCIDSQfzQn0sz5)
[SX1272 details semtech](https://semtech.my.salesforce.com/sfc/p/#E0000000JelG/a/440000001NCE/v_VBhk1IolDgxwwnOpcS_vTFxPfSEPQbuneK3mWsXlU)
[LoRa Calculator](https://www.semtech.com/design-support/lora-calculator)
[Vídeo explicación LoRa Modulation](https://youtu.be/jHWepP1ZWTk?si=Cj_pdeAbte_X_fY5)
[Nice Explanation LoRa web](https://www.sghoslya.com/p/how-does-lorawan-nodes-changes-their.html)
[Nice list of YT tutorials of LoRa](https://youtube.com/playlist?list=PLmL13yqb6OxdeOi97EvI8QeO8o-PqeQ0g&si=ScAtHjRUzWniHnis)

## Important Terms

1. Spreading Factor (SF): How much we "spread" our data over time. From 7-12. Determines the number of bits per symbol.
2. Receiver bandwidth (BW): How big of a slice of the spectrum we use. Typical values in EU: 125kHz, 250kHz and 500kHz.
3. Receiver noise figure (NF): Fixed for a given hardware implementation. The amount of noise introduced by the receiver difference of the ideal one.
4. Sound to noise ratio (SNR)
5. Forward Error Correction (FEC): Uses Hamming Coding, adding redundancy in order to check errors.
6. Sensitivity (S): Minimum power detectable.
7. Time On Air (ToA): Time duration of the packet. Affects energy consumption, duty cycle and latency. 
8. Chip Rate: Equals to bandwidth in chips-per-second-per-Hertz (125 kHz = 125 kcps).
9. Coding Rate (CR): How much redundancy we encode to resist noise.
10. Link budget: Determines the theoretical maximum range.
11. Data Rate
12. Adaptative Data Rate (ADR)


## Firmware things

[Official web for LoRa 32](https://heltec.org/project/wifi-lora-32-v3/)


# Comparison of mesh algorithms
[Dynamic Routing Algorithms](https://www.sciencedirect.com/science/article/abs/pii/S138912861100377X?utm_source=chatgpt.com)


| Algorithm           | Delivery ratio | Overhead | Scalability  | Comentario clave                       |
| ------------------- | -------------- | -------- | ------------ | -------------------------------------- |
| Flooding            | High           | High     | Bad         | Entrega masiva, pero congestión real   |
| Greedy (simple)     | Medium         | Bajo     | Good        | Eficiente, falla si hay voids          |
| Greedy+fallback     | High           | Medium   | Good        | Soluciona mínimos locales              |
| AODV / Reactive     | High           | Medium   | Good        | Balance clásico                        |
| Proactivo (OLSR)    | Variable       | High     | Medium      | Poco eficiente con duty limits         |
| Multi‑path / Hybrid | High           | Medium   | Good        | Alta robustez sin ineficiencia extrema |



---


# Comparison of Mesh Algorithms

[Classification of Routing Algorithms (GeeksForGeeks)](https://www.geeksforgeeks.org/computer-networks/classification-of-routing-algorithms/)

There are 3 types:
- Adaptative Routing Algorithms
- Non-Adaptative Routing Algorithms
- Hybrid Routing Algorithms

## Adaptative Routing Algorithms

- Dynamic routing: change routing decision whenever networks topology (both physical and logical connections) or traffic load changes.

### Types
1. Isolated: Makes decision using info. has.
2. Centralized: centralized node has entire information of the network.
3. Distributed: Receives info. from neighbors and takes decision.

## Non-Adaptative Algorithms

- **Do not** change their routing decision once they have been selected, a.k.a. **static routing**

### Types
- Flooding: adapts technique in which every incoming packet is sent.
- Random walk: sent neighboor-by-neighbor randomly.


## Hybrid Algorithms

- Combines both previous in distinct regions:
  - Link-state: Each router creates a detailed map of the network.
  - Distance vector: Each router maintains a table that contains information about the dist. and dir. to every other node.

## Additional Information
Flooding is preferred because adapts more to the environment the problem states for this bachelor's thesis.


## Algorithms to Implement

### Flooding / Managed Flooding
### Gossip / Probabilistic Flooding
### Distance Vector
### Hierarchical / Clustered-Based Routing
### Reverse Path Forwarding (RPF) / Gradient Routing
### Energy-Aware Routing
### Neighbor-Based / Local Decision Routing
### Beacon / Synchronization-Based Routing
### Multipath Routing
### Flooding + Memory (Hybrid)
### Spreading-Factor-Aware Routing


[ps circuit](https://forum.arduino.cc/t/esp32-heltec-wifi-kit-32-v2-fried/942179)


[Reyax Module to Copy](https://reyax.com/products/RYLR999)

[video Disaster Relief Messenger](https://www.youtube.com/watch?v=9_yHu9rJt58&t=825s)

---

## T-Echo device (interesant)

Links:
[T-Echo accessories](https://lilygo.cc/en-us/products/t-echo-accessories-1)
[T-Echo image](https://lilygo.cc/en-us/products/t-echo-accessories-1)
[T-Echo model 3D](https://cults3d.com/es/modelo-3d/artilugios/lilygo-t-echo-bigger-back-cover?srsltid=AfmBOoryAgTF_zkb5hv5yAtpenR4TBQ2V1CNmBqWtb5l3sxHU1m5CthJ)
[T-Echo video](https://www.youtube.com/watch?v=6KkTvHnoo_0)