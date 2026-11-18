#!/usr/bin/env python3
"""GENRA TECHNO MOD (30 HP): writes res/TechnoMod.svg, src/TechnoModLayout.hpp, docs/panels/technomod.png."""
from tpanel import *
import math

P = Panel("TechnoMod", 30, "MOD", "SONG MODULATION")
W = P.W
LANES = [("A", "#ff6b6b"), ("B", "#3dc9ff"), ("C", "#b8f23a"), ("D", "#e0779e")]
P.field("F_PRESET", 66, 3.8, 66, 6.4, "PRESET  PEAK TIME SWEEPS")
P.area("A_LINK", W - 18, 3.8, 13, 6.4)
P.display("DISP_MOD", 5, 15, W - 10, 31)

# ---- the matrix, turned: the song's sections side by side (as ABLAUF has them), the lanes A–D as rows
mx, my, mw, mh = 5, 48.5, 86, 51
P.section(mx, my, mw, mh, "MOVE PER SECTION")
SECS = [("INTRO", 0), ("GROOVE", 1), ("BUILD 1", 2), ("DROP 1", 3), ("BREAK 1", 4), ("BUILD 2", 2), ("DROP 2", 3), ("BREAK 2", 4), ("OUTRO", 5)]
PCOL = ["#8a93a6", "#7b6cff", "#e3e36a", "#ff4a4a", "#3dc9ff", "#8a93a6"]
CELLS = {0: ["RISE", "LFO", "RISE", "IN", "FALL", "RISE", "IN", "PART", "FALL"],
         1: ["OFF", "OFF", "RISE", "LFO", "SLOW", "RISE", "LFO", "OFF", "OFF"],
         2: ["OFF", "SLOW", "RISE", "IN", "OFF", "RISE", "IN", "SLOW", "OFF"],
         3: ["SLOW", "LFO", "OFF", "PUMP", "SLOW", "OFF", "PUMP", "IN", "FALL"]}
x0, colw, rowh, g = mx + 11.5, (mw - 13.5) / 9, 7.6, 0.8
for k, (nm, pi) in enumerate(SECS):
    xx = x0 + k * colw
    P.rect(xx + 0.3, my + 7.0, colw - 0.6, 3.6, PCOL[pi], 0.4, preview=True)
    P.text(xx + colw / 2, my + 9.6, nm, 0.95, "#07080a", 900, check=False, preview=True)
for j, (n, c) in enumerate(LANES):
    yy = my + 12.6 + j * (rowh + g)
    P.jack(f"IN_{n}", mx + 5.0, yy + rowh / 2, "")
    P.text(mx + 9.6, yy + rowh / 2 + 0.6, n, 1.5, c, 900)
    for k in range(9):
        v = CELLS[j][k]
        xx = x0 + k * colw
        if v == "PART":
            P.rect(xx + 0.3, yy, colw - 0.6, rowh, PCOL[SECS[k][1]], 0.6, "#ffffff", 0.4, preview=True)
            P.text(xx + colw / 2, yy + rowh / 2 + 0.5, "IN " + SECS[k][0].replace("BREAK", "BRK"), 0.95, "#07080a", 900, check=False, preview=True)
        elif v == "IN":
            P.rect(xx + 0.3, yy, colw - 0.6, rowh, c, 0.6, "#ffffff", 0.4, preview=True)
            P.text(xx + colw / 2, yy + rowh / 2 + 0.5, "IN", 1.2, "#07080a", 900, check=False, preview=True)
        else:
            P.rect(xx + 0.3, yy, colw - 0.6, rowh, mix(c, 0.35) if v != "OFF" else "#1a1b20", 0.6, preview=True)
            P.text(xx + colw / 2, yy + rowh / 2 + 0.5, v, 1.0, TXT if v != "OFF" else "#4a4c57", 800, check=False, preview=True)
P.text(mx + 3, my + mh - 3.0, "IN = THE LANE'S JACK  ·  IN BRK 2 = THE SECTION'S JACK  ·  WHEEL ON THE HEAD: MORE", 1.05, DIM, 700, "start")
P.area("A_MATRIX", x0, my + 7.0, 9 * colw, 4 * (rowh + g) + 5.6)
# the display (mock): the same columns, the lanes' curves, what comes in (dashed)
dx0, dy0, dw, dh = 7, 17, W - 14, 27
xx = dx0
for k, (nm, pi) in enumerate(SECS):
    w = dw / 9
    P.text(xx + w / 2, dy0 + 2.0, nm, 1.0, PCOL[pi], 800, check=False, preview=True)
    if k:
        P.p(f'<line x1="{xx:.2f}" y1="{dy0 + 3}" x2="{xx:.2f}" y2="{dy0 + dh}" stroke="#2a2c34" stroke-width="0.25"/>')
    xx += w
def path(f, col, extra=""):
    pts = []
    for i in range(361):
        t = i / 360
        pts.append(f"{dx0 + t * dw:.2f},{dy0 + dh - 1 - f(t) * (dh - 6):.2f}")
    P.p(f'<polyline points="{" ".join(pts)}" fill="none" stroke="{col}" stroke-width="0.45" {extra}/>')
ext = lambda t: 0.5 + 0.42 * math.sin(t * 2 * math.pi * 30)
def lane(cells):
    def f(t):
        k = min(8, int(t * 9)); u = t * 9 - k
        v = cells[k]
        return {"RISE": 0.15 + 0.7 * u, "FALL": 0.85 - 0.7 * u, "LFO": 0.5 + 0.3 * math.sin(u * 18), "SLOW": 0.4 + 0.15 * math.sin(u * 6),
                "PUMP": 0.3 + 0.5 * abs(math.sin(u * 25)), "IN": ext(t), "PART": 0.5 + 0.4 * math.sin(t * 2 * math.pi * 8), "OFF": 0.05}[v]
    return f
path(ext, "#ffffff", 'stroke-dasharray="0.6 0.5" stroke-opacity="0.5"')
path(lane(CELLS[0]), LANES[0][1])
path(lane(CELLS[2]), LANES[2][1])

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

# ---- jacks: the song's parts as inputs (row 1), the switches, the outputs on the right (row 2)
P.strip()
Y1, Y2 = 108.6, 121.4
x = 9.0
for k, (n, s_) in enumerate((("IN_CLK", "CLK"), ("IN_RST", "RST"), ("IN_PART", "PART"), ("IN_DEPTH", "DEPTH"))):
    P.jack(n, x + k * J, Y1, s_)
PN = [("INTRO", 0), ("GROOVE 1", 1), ("GROOVE 2", 1), ("BUILD 1", 2), ("BUILD 2", 2), ("DROP 1", 3), ("DROP 2", 3),
      ("BREAK 1", 4), ("BREAK 2", 4), ("OUTRO", 5)]
px0 = x + 4 * J + 2.0
for k, (pn, pi) in enumerate(PN):
    P.jack(f"IN_SEC{k}", px0 + k * J, Y1, "")
    P.area(f"A_SL{k}", px0 + k * J - 4.6, Y1 + 3.9, 9.2, 2.6)  # the module writes the name (bright: in the song)
    P.text(px0 + k * J, Y1 + 5.6, pn, 1.3, DIM, 700, check=False, preview=True)
    P.rect(px0 + k * J - 3.0, Y1 - 4.6, 6.0, 0.5, PCOL[pi], 0.2)
for k, (n, s_) in enumerate((("BTN_GROW", "GROW"), ("BTN_CHORD", "CHORD"), ("BTN_FREEZE", "FREEZE"))):
    P.button(n, 5.5 + k * 15.5, Y2 - 3.6, 14.5, 7.2, s_, mix(RED, 0.5) if k < 2 else "#2c2e36", "#ffffff" if k < 2 else TXT, 1.3)
ox = W - 9.0 - 6 * J
for k, (n, c) in enumerate(LANES):
    P.jack(f"OUT_{n}", ox + k * J, Y2, f"OUT {n}", mix(c), True)
for k, (n, s_) in enumerate((("OUT_PHRASE", "PHRASE"), ("OUT_PART", "PART OUT"), ("OUT_BAR", "BAR"))):
    P.jack(n, ox + (4 + k) * J, Y2, s_, "#2c2e36", True)

P.finish(1300)
