#!/usr/bin/env python3
"""GENRA TECHNO KUGEL (24 HP): writes res/TechnoKugel.svg, src/TechnoKugelLayout.hpp, docs/panels/technokugel.png."""
import math
from tpanel import *

AC = "#5ec8e6"  # KUGEL's accent (light blue)
P = Panel("TechnoKugel", 24, "KUGEL", "XY PHYSICS")
W = P.W
fx = P.titleEnd + 3
P.field("F_PRESET", fx, 3.8, W - fx - 19, 6.4, "PRESET  PINBALL", AC)
P.area("A_LINK", W - 17, 3.8, 12, 6.4)

# ---- display: the pad (square) and the values
px, py, ps = 5, 14.5, 44
P.display("DISP_PAD", px, py, ps, ps)
P.display("DISP_INFO", px + ps + 2, py, W - 10 - ps - 2, ps)
for i in range(1, 4):
    P.p(f'<line x1="{px + ps * i / 4:.2f}" y1="{py + 1}" x2="{px + ps * i / 4:.2f}" y2="{py + ps - 1}" stroke="#1f2a31" stroke-width="0.22"/>')
    P.p(f'<line x1="{px + 1}" y1="{py + ps * i / 4:.2f}" x2="{px + ps - 1}" y2="{py + ps * i / 4:.2f}" stroke="#1f2a31" stroke-width="0.22"/>')
Q = lambda a, b: f"{px + ps * a:.2f},{py + ps * b:.2f}"
P.p(f'<path d="M{Q(0.10, 0.32)} Q{Q(0.30, 0.06)} {Q(0.36, 0.96)} Q{Q(0.50, 0.12)} {Q(0.64, 0.96)} Q{Q(0.76, 0.42)} {Q(0.84, 0.62)}" '
    f'fill="none" stroke="{AC}" stroke-opacity="0.45" stroke-width="0.5"/>')
bx, by = px + ps * 0.84, py + ps * 0.62
P.p(f'<circle cx="{bx:.2f}" cy="{by:.2f}" r="2.7" fill="{AC}" fill-opacity="0.22"/><circle cx="{bx:.2f}" cy="{by:.2f}" r="1.7" fill="{AC}"/>')
cx0, cy0 = px + ps / 2, py + 5
P.p(f'<g transform="rotate(20 {cx0:.2f} {cy0:.2f})"><line x1="{cx0:.2f}" y1="{cy0 - 2.5:.2f}" x2="{cx0:.2f}" y2="{cy0 + 2.5:.2f}" '
    f'stroke="#8a8d99" stroke-width="0.35"/><polyline points="{cx0 - 1:.2f},{cy0 + 1.3:.2f} {cx0:.2f},{cy0 + 2.5:.2f} {cx0 + 1:.2f},{cy0 + 1.3:.2f}" '
    f'fill="none" stroke="#8a8d99" stroke-width="0.35"/></g>')
ix, iw = px + ps + 4.5, W - 10 - ps - 2 - 5
ROWS = [("GRAVITY", "0.62 g"), ("TILT", "20°"), ("FRICTION", "18 %"), ("MASS", "1.0"), ("BOUNCE", "74 %"), ("WALL", "BOUNCE"),
        ("LOOP", "PLAY 3 / 8"), ("X", "+2.84 V"), ("Y", "-3.10 V"), ("HIT", "●")]
for i, (k, v) in enumerate(ROWS):
    yy = py + 4.6 + i * 4.1
    P.text(ix, yy, k, 1.35, DIM, 700, "start", check=False, preview=True)
    P.text(ix + iw, yy, v, 1.5, AC if k in ("X", "Y", "HIT") else TXT, 700, "end", check=False, preview=True)

# ---- PHYSICS
sy, sh = 61, 38.4
P.section(5, sy, 56, sh, "PHYSICS", AC)
P.knob("K_GRAV", 16, sy + 14.5, "large", AC, "GRAVITY")
# TILT: endless (no pointer, the angle in the middle, an arc)
tx, ty, d = 34, sy + 12.5, KNOB_D["medium"]
P.ring(tx, ty, d, "#555a66")
P.text(tx, ty + d / 2 + 3.2, "TILT", 1.35, DIM, 700)
P.comp("knob", "K_TILT", tx, ty)
P.p(f'<circle cx="{tx}" cy="{ty}" r="{d / 2}" fill="#0e0f12"/><circle cx="{tx}" cy="{ty}" r="{d / 2 * 0.86:.2f}" fill="#1d1f25"/>')
ar, ang = d / 2 - 1.3, math.radians(20)
P.p(f'<path d="M{tx},{ty - ar:.2f} A{ar:.2f},{ar:.2f} 0 0 1 {tx + math.sin(ang) * ar:.2f},{ty - math.cos(ang) * ar:.2f}" fill="none" stroke="{AC}" stroke-width="0.6" stroke-linecap="round"/>')
P.text(tx, ty + 0.6, "20°", 1.7, TXT, 800, check=False, preview=True)
P.knob("K_FRIC", 51, sy + 12.5, "medium", None, "FRICTION")
P.knob("K_MASS", 34, sy + 28.8, "small", None, "MASS")
P.knob("K_BNC", 51, sy + 28.8, "small", None, "BOUNCE")

# ---- MOTION
mx, mw = 63, W - 68
P.section(mx, sy, mw, sh, "MOTION", AC)
fw = (mw - 4 - 2 * 1.4) / 3
for k, (n, v, lb) in enumerate((("F_WALL", "BOUNCE", "WALL"), ("F_RANGE", "±5 V", "RANGE"), ("F_LOOP", "2 BARS", "LOOP LEN"))):
    x = mx + 2 + k * (fw + 1.4)
    P.field(n, x, sy + 6.6, fw, 5.0, v, AC)
    P.text(x + fw / 2, sy + 14.6, lb, 1.25, DIM, 700)
bw = (mw - 4 - 3 * 1.2) / 4
for k, (n, lb, fill, tc) in enumerate((("BTN_KICK", "KICK", "#c8641e", "#ffffff"), ("BTN_HOLD", "HOLD", "#2c2e36", TXT),
                                     ("BTN_REC", "REC", "#a83232", "#ffffff"), ("BTN_LOOP", "LOOP", "#2e7d4a", "#ffffff"))):
    P.button(n, mx + 2 + k * (bw + 1.2), sy + 16.6, bw, 6.0, lb, fill, tc, 1.35)
P.knob("K_FORCE", mx + mw * 0.28, sy + 29.6, "small", None, "FORCE", 2.6)
P.knob("K_SMOOTH", mx + mw * 0.72, sy + 29.6, "small", None, "SMOOTH", 2.6)

# ---- jacks
P.strip()
Y1, Y2 = 108.6, 121.4
row1 = [("IN_GATE", "GATE"), ("IN_VEL", "VEL"), None, ("IN_CLK", "CLK"), ("IN_RST", "RST"), None,
        ("IN_GRAV", "GRAV"), ("IN_TILT", "TILT"), ("IN_FRIC", "FRIC"), ("IN_BNC", "BOUNCE")]
x = 9.0
for it in row1:
    if it is None:
        x += 2.0
        continue
    P.jack(it[0], x, Y1, it[1])
    if it[0] in ("IN_GATE", "IN_VEL"):
        P.rect(x - 3.0, Y1 - 4.6, 6.0, 0.5, mix(AC, 0.45, "#141519"), 0.2)
    elif it[0] in ("IN_CLK", "IN_RST"):
        P.rect(x - 3.0, Y1 - 4.6, 6.0, 0.5, "#5a5c66", 0.2)
    x += J
for k, (n, lb) in enumerate((("IN_PUSHX", "PUSH X"), ("IN_PUSHY", "PUSH Y"), ("IN_REC", "REC"))):
    P.jack(n, 9.0 + k * J, Y2, lb)
ox = W - 9.0 - 3 * J
for k, (n, lb) in enumerate((("OUT_X", "X"), ("OUT_Y", "Y"), ("OUT_SPD", "SPEED"), ("OUT_HIT", "HIT"))):
    P.jack(n, ox + k * J, Y2, lb, mix(AC, 0.32) if k < 3 else "#2c2e36", True)
P.finish(1400)
