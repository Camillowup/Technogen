#!/usr/bin/env python3
"""GENRA TECHNO MOD (30 HP): writes res/TechnoMod.svg, src/TechnoModLayout.hpp, docs/panels/technomod.png."""
from tpanel import *

P = Panel("TechnoMod", 30, "MOD", "SONG MODULATION")
W = P.W
LANES = [("A", "#ff6b6b"), ("B", "#3dc9ff"), ("C", "#b8f23a"), ("D", "#e0779e")]
P.field("F_PRESET", 66, 3.8, 66, 6.4, "PRESET  PEAK TIME SWEEPS")
P.area("A_LINK", W - 18, 3.8, 13, 6.4)
P.display("DISP_MOD", 5, 15, W - 10, 31)

# ---- the matrix: one movement per part and lane
mx, my, mw, mh = 5, 48.5, 86, 51
P.section(mx, my, mw, mh, "MOVE PER PART")
cx0, cw, ch, gap = mx + 17.5, 15.0, 5.7, 1.1
for j, (n, c) in enumerate(LANES):
    P.text(cx0 + j * (cw + gap) + cw / 2, my + 8.8, n, 1.5, c, 800)
P.area("A_MATRIX", mx + 2.5, my + 10.8, cx0 + 4 * (cw + gap) - gap - (mx + 2.5), 6 * (ch + gap) - gap)

# ---- the lanes
lx, ly, lw, lh = 93, 48.5, W - 98, 51
P.section(lx, ly, lw, lh, "LANES")
colw = lw / 4
for j, (n, c) in enumerate(LANES):
    x = lx + colw * (j + 0.5)
    P.text(x, ly + 8.4, n, 1.5, c, 800)
    P.knob(f"K_DEPTH{j}", x, ly + 15.4, "medium", c, "DEPTH")
    P.knob(f"K_SMOOTH{j}", x, ly + 28.6, "small", None, "SMOOTH")
    P.field(f"F_TARGET{j}", x - colw / 2 + 0.8, ly + 37.4, colw - 1.6, 5.0, "CUTOFF", c)
    P.field(f"F_BARS{j}", x - colw / 2 + 0.8, ly + 43.6, colw * 0.55 - 1.0, 5.0, "16", c)
    P.button(f"BTN_UNI{j}", x - colw / 2 + colw * 0.55 + 0.4, ly + 43.6, colw * 0.45 - 1.2, 5.0, "UNI", "#2c2e36", TXT, 1.2)

# ---- jacks and the three switches
P.strip()
Y1, Y2 = 108.6, 121.4
x = 9.0
for k, (n, s) in enumerate((("IN_CLK", "CLK"), ("IN_RST", "RST"), ("IN_PART", "PART"), ("IN_DEPTH", "DEPTH"))):
    P.jack(n, x + k * J, Y1, s)
for k, (n, s) in enumerate((("BTN_GROW", "GROW"), ("BTN_CHORD", "CHORD"), ("BTN_FREEZE", "FREEZE"))):
    P.button(n, W - 61 + k * 19, Y1 - 3.6, 17.5, 7.2, s, mix(RED, 0.5) if k < 2 else "#2c2e36", "#ffffff" if k < 2 else TXT, 1.4)
P.rect(x - 4.3, 115.9, 4 * J - 0.6, 0.5, RED, 0.2)
for k, (n, c) in enumerate(LANES):
    P.jack(f"OUT_{n}", x + k * J, Y2, f"OUT {n}", mix(c), True)
x = 9.0 + 4 * J + 3.0
for k, (n, s) in enumerate((("OUT_PHRASE", "PHRASE"), ("OUT_PART", "PART"), ("OUT_BAR", "BAR"))):
    P.jack(n, x + k * J, Y2, s, "#2c2e36", True)

P.finish(1300)
