# A3MESH — Thesis Manuscript and Umbrella Repository

This repository hosts the LaTeX source of the Bachelor's Thesis
*A3MESH: A Decentralised LoRa Mesh Platform for Off-Grid Smartphone
Messaging* and acts as the top-level container for the four artefacts
that make up the project. Each artefact is included here as a git
submodule, so that a single recursive clone retrieves the complete
project tree — manuscript, firmware, mobile application, hardware
design, and mechanical enclosure.

## Repository Layout

```
tfg_manuscript/
├── tfe.tex                       Main LaTeX source
├── tfe.bib                       BibTeX bibliography (biber backend)
├── tfe.pdf                       Compiled pdf
├── tfgitic.cls                   UPC/ITIS thesis document class
├── custom_cmd.sty                Custom LaTeX macros
├── images/                       Figures embedded in the document
├── tfg_firmware/     [submodule] ESP32 node firmware
├── tfg_hardware/     [submodule] KiCad PCB design
├── tfg_software/     [submodule] Android companion application
├── tfg_3d/           [submodule] Mechanical enclosure (Fusion 360)
└── MeshtasticatorMeshAlgoComp/
                      [submodule] Network simulator (Smart Gossip)
```

The compiled artefact is `tfe.pdf`. Appendix D of the manuscript
mirrors this layout and provides a per-submodule directory guide;
individual submodules ship their own `README.md` with build
instructions.

## Cloning

```bash
git clone --recurse-submodules https://github.com/alcaaron/tfg_manuscript.git
cd tfg_manuscript
```

If the repository was cloned without submodules, initialise them
retroactively with:

```bash
git submodule update --init --recursive
```

## Building the Manuscript

The document targets a standard TeX Live 2023 installation with
`biber` as the bibliography backend. The recommended build command
handles all cross-references, the biblatex pass, and the necessary
re-runs automatically:

```bash
latexmk -pdf tfe.tex
```

For a manual build:

```bash
pdflatex tfe.tex
biber    tfe
pdflatex tfe.tex
pdflatex tfe.tex
```

Auxiliary files may be removed with `latexmk -C`. The document uses
the `tfgitic` class provided by the Universitat Politècnica de
Catalunya — Manresa (School of Engineering); the class file is
included in the repository and requires no external installation.

## Project Overview

A3MESH is a decentralised mesh network for text messaging that
operates independently of cellular, Wi-Fi, or internet connectivity.
Each node combines an ESP32 microcontroller with a Semtech SX1276
LoRa transceiver on a custom two-layer PCB and communicates with the
user's smartphone over Bluetooth Low Energy. Messages traverse the
mesh by means of *Smart Gossip*, an adaptive probabilistic forwarding
algorithm developed and tuned within the scope of this thesis to
reduce channel occupation and end-to-end latency relative to managed
flooding, at the cost of a modest and quantifiable reduction in
reachability.

The four submodules materialise the four engineering disciplines
covered by the thesis:

- **`tfg_firmware/`** — C++17 firmware for the ESP32, implementing the
  LoRa driver, BLE Nordic UART Service, AES-256-GCM cryptographic
  layer, Smart Gossip routing, and the AT-command interface exposed to
  the mobile application.
- **`tfg_software/`** — Kotlin Android application providing a
  conversation-style user interface, contact management, key
  provisioning, and the BLE bridge to the paired node.
- **`tfg_hardware/`** — KiCad 8 project of the custom PCB, including
  schematic, layout, custom symbol and footprint libraries, and the
  Gerber and pick-and-place files used for production at JLCPCB.
- **`tfg_3d/`** — Autodesk Fusion 360 enclosure that permanently bonds
  the assembled node to the back of a smartphone case.

## Reading Order for Developers

For a reader approaching the code base for the first time, the
following chapter order in the manuscript minimises back-references:

1. **Chapter 1 — Introduction** and **Chapter 2 — Background** for
   the problem statement and the underlying LoRa, BLE, and mesh
   concepts.
2. **Chapter 3, Section 1 — System Architecture** for the whole-system view and
   the interaction between the node, the smartphone, and the mesh.
3. **Chapter 3, Section  4 — Hardware** and **Section 5 — Firmware** for the
   design and implementation of a single node.
4. **Chapter 3, Section  6 — Mobile Application** for the companion app.
5. **Chapter 3, Section  3 — Simulation** for the design and optimisation of the
   Smart Gossip algorithm.
6. **Chapter 3, Section  7 — Results** and **Chapter 11 — Conclusions** for the
   experimental validation and closing discussion.

## Author and Licence

Aarón Alcalá — Bachelor's Thesis in ICT Systems Engineering, Universitat Politècnica de Catalunya, Escola Politècnica
Superior d'Enginyeria de Manresa (EPSEM), 2026.

The manuscript is distributed under the terms indicated in the
front matter of `tfe.pdf`. Each submodule ships with its own licence
file where applicable.
