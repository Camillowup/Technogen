#!/usr/bin/env python3
"""GENRA TECHNO ABLAUF (34 HP): writes res/TechnoAblauf.svg, src/TechnoAblaufLayout.hpp, docs/panels/technoablauf.png."""
from tpanel import *

P = Panel("TechnoAblauf", 34, "ABLAUF", "SONG ARRANGER")
W = P.W
P.area("A_LINK", W - 30, 4.5, 26, 5.5)

# ---- SONG
sx, sy, sw, sh = 5, 14, 76, 84
P.section(sx, sy, sw, sh, "SONG", left=True)
P.button("BTN_PATT", sx + sw - 30, sy + 1.4, 12.5, 4.6, "PATT", "#2c2e36", DIM, 1.2)
P.button("BTN_SONG", sx + sw - 16.5, sy + 1.4, 13.5, 4.6, "SONG", mix(RED, 0.6), "#ffffff", 1.2)
P.display("DISP_SONG", sx + 2, sy + 7.5, sw - 4, 44)
P.knob("K_ROW", sx + 9, sy + 59.0, "large", RED, "ROW")
P.knob("K_PAD", sx + 24, sy + 59.0, "small", None, "PAD", 3.3 + (12.19 - 7.68) / 2)
P.knob("K_BARS", sx + 37, sy + 59.0, "small", None, "BARS", 3.3 + (12.19 - 7.68) / 2)
P.knob("K_PART", sx + 50, sy + 59.0, "small", None, "PART", 3.3 + (12.19 - 7.68) / 2)
P.button("BTN_PLAY", sx + 58, sy + 54.8, 16, 8.4, "PLAY", RED, "#ffffff", 1.4)
bw = (sw - 4 - 4 * 1.0) / 5
for i, (n, s) in enumerate((("BTN_INSERT", "INSERT"), ("BTN_DELETE", "DELETE"), ("BTN_UNDO", "UNDO"),
                            ("BTN_STOP", "STOP"), ("BTN_LOOP", "LOOP"))):
    P.button(n, sx + 2 + i * (bw + 1.0), sy + 69.6, bw, 6.4, s, "#2c2e36", TXT, 1.15)
P.text(sx + 2, sy + 81.3, "MIDI", 1.2, DIM, 700, "start", 0.3)
P.button("BTN_EXPSONG", sx + 10, sy + 77.6, 30, 5.6, "EXPORT SONG", "#2c2e36", TXT, 1.15)
P.button("BTN_EXPPAD", sx + 42, sy + 77.6, 32, 5.6, "EXPORT PAD", "#2c2e36", TXT, 1.15)

# ---- PATTERNS
px, py, pw, ph = 83, 14, W - 88, 84
P.section(px, py, pw, ph, "PATTERNS", left=True)
for i, b in enumerate("ABCD"):
    P.button(f"BTN_BANK{i}", px + pw - 37 + i * 8.6, py + 1.4, 7.6, 4.6, b, "#2c2e36", DIM, 1.3)
gap = 1.4
pd = (pw - 4 - 3 * gap) / 4
ph_ = pd * 0.55
P.area("A_PADS", px + 2, py + 8, pw - 4, 4 * ph_ + 3 * gap)
vy = py + 8 + 4 * (ph_ + gap) + 1.2
P.text(px + 2, vy + 1.0, "VARIATION · PRESS AGAIN = NEXT VARIANT", 1.05, DIM, 700, "start", 0.2)
bw2 = (pw - 4 - 5 * 1.0) / 6
for i in range(6):
    P.button(f"BTN_VAR{i}", px + 2 + i * (bw2 + 1.0), vy + 2.6, bw2, 7.0, ["INTRO", "GROOVE", "BUILD", "DROP", "BREAK", "OUTRO"][i],
             mix(PART_COLS[i], 0.55), "#ffffff", 1.05)
fy = vy + 11.6
P.field("F_FORM", px + 2, fy, pw - 53, 5.6, "FORM  PEAK TIME")
P.button("BTN_TAKE", px + pw - 50, fy, 23, 5.6, "TAKE LIVE", mix(RED, 0.6), "#ffffff", 1.2)
P.button("BTN_MAKE", px + pw - 26, fy, 24, 5.6, "MAKE SONG", RED, "#ffffff", 1.2)
by = fy + 7.0
bw3 = (pw - 4 - 3 * 1.0) / 4
for i, (n, s) in enumerate((("BTN_DJ", "DJ IN/OUT"), ("BTN_COPY", "COPY"), ("BTN_CLEAR", "CLEAR"), ("BTN_CLEARALL", "CLEAR ALL"))):
    P.button(n, px + 2 + i * (bw3 + 1.0), by, bw3, 5.2, s, "#2c2e36", TXT, 1.05)
if by + 5.2 > py + ph - 0.5:
    raise SystemExit("ABLAUF: the last row of buttons is past the section")

# ---- jacks
P.strip()
Y1, Y2 = 108.6, 121.4
x = 9.0
for k, (n, s) in enumerate((("IN_RUN", "RUN"), ("IN_RST", "RST"), ("IN_NEXT", "NEXT"), ("IN_PADCV", "PAD CV"), ("IN_ROWCV", "ROW CV"))):
    P.jack(n, x + k * J, Y1, s)
for k, (n, s) in enumerate((("OUT_END", "END"), ("OUT_ROW", "ROW"), ("OUT_PART", "PART"), ("OUT_BUILD", "BUILD"),
                            ("OUT_DROP", "DROP"), ("OUT_PHRASE", "PHRASE"))):
    P.jack(n, x + k * J, Y2, s, mix(RED), True)

P.finish(1500)
