#!/usr/bin/env python3
"""GENRA TECHNO ABLAUF (34 HP): writes res/TechnoAblauf.svg, src/TechnoAblaufLayout.hpp, docs/panels/technoablauf.png."""
from tpanel import *

P = Panel("TechnoAblauf", 34, "ABLAUF", "SONG ARRANGER")
W = P.W
P.area("A_LINK", W - 30, 4.5, 26, 5.5)

# ---- the song's timeline (as in GENRA TECHNO's display): every row a block in its part's colour, the playhead
P.display("DISP_TIME", 5, 13.4, W - 10, 9.0)
tl = [("INTRO", 8), ("GROOVE", 16), ("BUILD", 8), ("DROP", 16), ("BREAK", 8), ("BUILD", 8), ("DROP", 32), ("OUTRO", 8)]
PI = {"INTRO": 0, "GROOVE": 1, "BUILD": 2, "DROP": 3, "BREAK": 4, "OUTRO": 5}
tot = sum(b for _, b in tl)
tx0, tww = 7.0, W - 14
xx = tx0
for k, (nm, b) in enumerate(tl):
    w = tww * b / tot - 0.5
    P.rect(xx, 15.2, w, 5.4, PART_COLS[PI[nm]], 0.5, preview=True)
    P.text(xx + w / 2, 18.4, nm if w > 9 else nm[:1], 1.1, "#07080a", 900, check=False, preview=True)
    xx += w + 0.5
P.rect(tx0 + tww * 44 / tot, 14.4, 0.5, 7.0, "#ffffff", 0.2, preview=True)

# ---- SONG (the rows: pick, change, edit; MIDI)
sx, sy, sw, sh = 5, 24.5, 76, 75.1
P.section(sx, sy, sw, sh, "SONG", left=True)
P.display("DISP_SONG", sx + 2, sy + 7.5, sw - 4, 35.5)
kx = [sx + 10, sx + 29, sx + 47.5, sx + 66]
P.knob("K_ROW", kx[0], sy + 50.5, "large", RED, "ROW")
P.knob("K_PAD", kx[1], sy + 50.5, "small", None, "PAD", 3.3 + (12.19 - 7.68) / 2)
P.knob("K_BARS", kx[2], sy + 50.5, "small", None, "BARS", 3.3 + (12.19 - 7.68) / 2)
P.knob("K_PART", kx[3], sy + 50.5, "small", None, "PART", 3.3 + (12.19 - 7.68) / 2)
bw = (sw - 4 - 2 * 1.0) / 3
for i, (n, s_) in enumerate((("BTN_INSERT", "INSERT"), ("BTN_DELETE", "DELETE"), ("BTN_UNDO", "UNDO"))):
    P.button(n, sx + 2 + i * (bw + 1.0), sy + 60.9, bw, 6.2, s_, "#2c2e36", TXT, 1.15)
P.text(sx + 2, sy + 72.8, "MIDI", 1.2, DIM, 700, "start", 0.3)
P.button("BTN_EXPSONG", sx + 10, sy + 69.1, 30, 5.6, "EXPORT SONG", "#2c2e36", TXT, 1.15)
P.button("BTN_EXPPAD", sx + 42, sy + 69.1, 32, 5.6, "EXPORT PAD", "#2c2e36", TXT, 1.15)

# ---- PATTERNS
px, py, pw, ph = 83, 24.5, W - 88, 75.1
P.section(px, py, pw, ph, "PATTERNS", left=True)
for i, b in enumerate("ABCD"):
    P.button(f"BTN_BANK{i}", px + pw - 37 + i * 8.6, py + 1.4, 7.6, 4.6, b, "#2c2e36", DIM, 1.3)
gap = 1.4
pd = (pw - 4 - 3 * gap) / 4
ph_ = pd * 0.47
P.area("A_PADS", px + 2, py + 8, pw - 4, 4 * ph_ + 3 * gap)
vy = py + 8 + 4 * (ph_ + gap) + 1.2
P.text(px + 2, vy + 1.0, "VARIATION · PRESS AGAIN = NEXT VARIANT", 1.05, DIM, 700, "start", 0.2)
bw2 = (pw - 4 - 5 * 1.0) / 6
for i in range(6):
    P.button(f"BTN_VAR{i}", px + 2 + i * (bw2 + 1.0), vy + 2.6, bw2, 7.0, ["INTRO", "GROOVE", "BUILD", "DROP", "BREAK", "OUTRO"][i],
             mix(PART_COLS[i], 0.55), "#ffffff", 1.05)
fy = vy + 11.6
P.field("F_FORM", px + 2, fy, pw - 53, 5.6, "FORM  PEAK TIME")
P.button("BTN_TAKE", px + pw - 50, fy, 23, 5.6, "TAKE LIVE", "#ff8c2e", "#ffffff", 1.2)
P.button("BTN_MAKE", px + pw - 26, fy, 24, 5.6, "MAKE SONG", "#ff8c2e", "#ffffff", 1.2)
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

# ---- TRANSPORT (in the jack strip, beside RUN / RST / NEXT)
tx, ty, tw, th = 66, 103.4, W - 71, 23.6
P.section(tx, ty, tw, th, "TRANSPORT", left=True)
P.button("BTN_PATT", tx + 2, ty + 7.6, 17, 6.6, "PATT", "#2c2e36", DIM, 1.2)
P.button("BTN_SONG", tx + 2, ty + 15.2, 17, 6.6, "SONG", mix("#3dc9ff", 0.55), "#ffffff", 1.2)
P.button("BTN_PLAY", tx + 22, ty + 7.6, 30, 14.2, "PLAY", "#2fb866", "#ffffff", 1.6)
P.button("BTN_STOP", tx + 54, ty + 7.6, 22, 14.2, "STOP", "#2c2e36", TXT, 1.4)
P.button("BTN_LOOP", tx + 78, ty + 7.6, tw - 80, 14.2, "LOOP", "#2c2e36", TXT, 1.4)

P.finish(1500)
