#!/usr/bin/env python3
"""GENRA TECHNO GRID (44 HP): writes res/TechnoGrid.svg, src/TechnoGridLayout.hpp, docs/panels/technogrid.png."""
from tpanel import *

P = Panel("TechnoGrid", 44, "GRID", "GRID EDITOR")
W = P.W
P.area("A_LINK", W - 50, 4.5, 46, 5.5)
P.display("DISP_GRID", 5, 14, W - 10, 80)

sy, sh = 97, 29
TR = [("DRUMS", DR), ("BASS", BA), ("STAB", ME), ("LEAD", LE), ("ATMO", PA)]
secs = [("TRACK", 58), ("VIEW", 38), ("NOTES", 62), ("EDIT", W - 10 - 58 - 38 - 62 - 6)]
x = 5
for name, w in secs:
    P.section(x, sy, w, sh, name)
    if name == "TRACK":
        bw = (w - 4 - 4 * 1.0) / 5
        for i, (n, c) in enumerate(TR):
            bx = x + 2 + i * (bw + 1.0)
            P.rect(bx, sy + 7, bw, 0.9, c, 0.3)
            P.button(f"BTN_TRK{i}", bx, sy + 9.5, bw, 8.4, n, "#2c2e36", TXT, 1.2)
        half = (w - 5) / 2
        P.button("BTN_REC", x + 2, sy + 20, half, 6.6, "REC", RED, "#ffffff", 1.3)
        P.button("BTN_SOLO", x + 3 + half, sy + 20, half, 6.6, "SOLO", "#2c2e36", TXT, 1.3)
    elif name == "VIEW":
        for i, (n, s) in enumerate((("K_BAR", "BAR"), ("K_ZOOM", "ZOOM"), ("K_PITCH", "PITCH"))):
            P.knob(n, x + 7 + i * 12, sy + 12.6, "small", None, s)
        P.button("BTN_VIEWMODE", x + 3, sy + 21, w - 6, 6.0, "ALL · SCALE · USED", "#2c2e36", TXT, 1.15)
    elif name == "NOTES":
        for i, (n, s) in enumerate((("K_VEL", "VEL"), ("K_NLEN", "LEN"), ("K_CHANCE", "CHANCE"), ("K_RATCH", "RATCH"))):
            P.knob(n, x + 8 + i * 15.3, sy + 12.6, "small", RED if s == "RATCH" else None, s)
        bw = (w - 4 - 3 * 1.0) / 4
        for i, (n, s) in enumerate((("BTN_ACC", "ACCENT"), ("BTN_SLIDE", "SLIDE"), ("BTN_SNAP", "SNAP"), ("BTN_FOLLOW", "FOLLOW"))):
            P.button(n, x + 2 + i * (bw + 1.0), sy + 21, bw, 6.0, s, "#2c2e36", TXT, 1.15)
    else:
        bw = (w - 4 - 2 * 1.2) / 3
        for i, (n, s) in enumerate((("BTN_UNDO", "UNDO"), ("BTN_REDO", "REDO"), ("BTN_CLEAR", "CLEAR"),
                                    ("BTN_COPY", "COPY"), ("BTN_PASTE", "PASTE"), ("BTN_NEWTRK", "NEW TRACK"))):
            bx = x + 2 + (i % 3) * (bw + 1.2)
            by = sy + 7 + (i // 3) * 10.5
            P.button(n, bx, by, bw, 8.4, s, RED if s == "NEW TRACK" else "#2c2e36", "#ffffff" if s == "NEW TRACK" else TXT, 1.25)
    x += w + 2

P.finish(1800)
