#!/usr/bin/env python3
"""
Generates a state-machine diagram for MeshRxState in the visual style of the
reference image (circular nodes, thin black strokes, curved labeled edges,
white background), laid out on an explicit grid using Graphviz's neato
engine with fixed node positions. Outputs a vector PDF suitable for
inclusion in a LaTeX document via \\includegraphics.
"""

import graphviz

dot = graphviz.Digraph(
    name="MeshRxState",
    format="png",
    engine="neato",
)

# ---- Global graph attributes -------------------------------------------------
dot.attr(
    bgcolor="white",
    splines="polyline",
    fontname="Helvetica",
    nodesep="1.1",
    ranksep="1.1",
)

# ---- Default node style: plain circle, like the reference image -------------
dot.attr(
    "node",
    shape="circle",
    style="filled",
    fillcolor="white",
    color="black",
    penwidth="1.2",
    fontname="Helvetica",
    fontsize="12",
    fixedsize="true",
    width="1.35",
    height="1.35",
)

# ---- Default edge style -------------------------------------------------------
dot.attr(
    "edge",
    color="black",
    penwidth="1.0",
    fontname="Helvetica",
    fontsize="11",
    arrowsize="0.7",
)

# ---- Nodes on a real 6-column x 3-row grid ------------------------------------
# Grid cell size: COL x ROW (inches). Row 0 = top, row 2 = bottom.
COL = 3.1
ROW = 2.6

def cell(col, row):
    """Map (column, row) grid coordinates to neato inches. Row 0 is the top
    row, so y is inverted (higher row index -> lower on the page)."""
    return (col * COL, -row * ROW)

# Column layout (0-indexed, 6 columns total):
#   0: idle   1: validate   2: classify/key   3: discov./data   4: fwd/local   5: done
#   Row 3 (extra bottom lane): drop, centered under classify/data
positions = {
    "idle":      cell(0, 1),
    "validate":  cell(1, 1),
    "classify":  cell(2, 1),
    "discovery": cell(3, 0),
    "data":      cell(3, 1),
    "key":       cell(3, 2),
    "forward":   cell(4, 0),
    "local":     cell(4, 1),
    "drop":      cell(3.5, 3),
    "done":      cell(5, 1),
}

labels = {
    "idle":      "k_rx_idle",
    "validate":  "k_rx_validate",
    "classify":  "k_rx_classify",
    "discovery": "k_rx_process\n_discovery",
    "data":      "k_rx_process\n_data",
    "key":       "k_rx_process\n_key",
    "local":     "k_rx_local\n_delivery",
    "forward":   "k_rx_forward",
    "drop":      "k_rx_drop",
    "done":      "k_rx_done",
}

for nid, (x, y) in positions.items():
    dot.node(nid, label=labels[nid], pos=f"{x},{y}!")

# ---- Edges: main forward flow ------------------------------------------------
dot.edge("idle", "validate", label="xQueueReceive()\nunblocks")
dot.edge("validate", "classify", label="deserialize_pkt() ok,\nttl>0, copies\u2264MAX")
dot.edge("classify", "discovery", label="header.type ==\nk_discovery_type")
dot.edge("classify", "data", label="header.type ==\nk_data_plain/encrypted")
dot.edge("classify", "key", label="header.type ==\nk_key_exchange")
dot.edge("data", "local", label="is_for_me &&\ncopies == 1")
dot.edge("data", "forward", label="should_forward()\n== true")
dot.edge("discovery", "done", tailport="n", headport="n")
dot.edge("local", "done")
dot.edge("forward", "done")

# ---- Edges: drop paths --------------------------------------------------------
dot.edge("validate", "drop", label="deserialize fail / own pkt /\nttl==0 / copies>MAX")
dot.edge("classify", "drop", label="default:\nunknown type")
dot.edge("data", "drop", label=" should_forward()\n== false")

# key->local is routed through an invisible waypoint placed well to the
# right (near local's column), so it swings clear of the data->drop edge
# (which occupies the x-band between data and drop) instead of crossing it.
wp_x, wp_y = positions["local"][0] - 0.3, positions["key"][1] + 0.9
dot.node("_wp1", label="", shape="point", width="0.01", style="invis", pos=f"{wp_x},{wp_y}!")
dot.edge("key", "_wp1", arrowhead="none")
dot.edge("_wp1", "local", label="is_for_me ")
dot.edge("key", "drop", label="!is_for_me")
dot.edge("local", "drop", label="CryptoError\n!= OK")
dot.edge("drop", "done")

# ---- Loop back: done -> idle, routed in its own lane well above discov./forward
dot.edge("done", "idle", label="for(;;) re-enters,\nblocks on xLoRaRxQueue", headport="n", tailport="n")

# ---- Render -------------------------------------------------------------------
output_path = dot.render(filename="mesh_rx_state_machine", directory="./images", cleanup=True)
print(f"Rendered: {output_path}")