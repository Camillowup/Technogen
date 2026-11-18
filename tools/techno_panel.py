#!/usr/bin/env python3
"""GENRA TECHNO, the main module (48 HP): writes res/GenraTechno.svg, src/GenraTechnoLayout.hpp, docs/panels/genratechno.png."""
from tpanel import *

P = Panel("GenraTechno", 48, "GENRA", "CLUB SEQUENCER · 5 TRACKS")
W = P.W

# ---- head: transport and the big knobs
P.button("BTN_STOP", 96, 3.6, 15, 7.2, "STOP")
P.button("BTN_PLAY", 113, 3.6, 15, 7.2, "PLAY", "#2c2e36", TXT)
P.knob("K_BPM", 140, 9.2, "medium", None, "BPM")
P.knob("K_SWING", 155, 9.2, "medium", None, "SWING")
P.knob("K_ENERGY", 174, 9.2, "large", RED, "ENERGY")
P.knob("K_EVOLVE", 194, 9.2, "large", RED, "EVOLVE")
P.knob("K_LENGTH", 216, 9.2, "medium", None, "LENGTH")

# ---- display and the choices under it
P.display("DISP_MAIN", 5, 21.5, 150, 25.5)
# keys in the display: a part back / on, MAIN (the drop in a loop) or the song
P.button("BTN_PREV", 80, 23.7, 12.5, 5.5, "< PART", "#2c2e36", TXT, 1.25)
P.button("BTN_NEXT", 93.5, 23.7, 12.5, 5.5, "PART >", "#2c2e36", TXT, 1.25)
P.button("BTN_MAIN", 108, 23.7, 13, 5.5, "MAIN", mix("#3dc9ff", 0.55), "#ffffff", 1.35)
P.button("BTN_SONGMODE", 122, 23.7, 15, 5.5, "LIVE SONG", "#2c2e36", DIM, 1.15)
P.field("F_FAMILY", 5, 50, 27, 5.6, "FAM  BERLIN")
P.field("F_STYLE", 34, 50, 37, 5.6, "STYLE  HYPNOTIC")
P.field("F_KEY", 73, 50, 19, 5.6, "KEY  A")
P.field("F_SCALE", 94, 50, 32, 5.6, "SCALE  AUTO", BA)
P.field("F_BUILD", 128, 50, 27, 5.6, "BUILD  AUTO", YEL)

# ---- PERFORM
P.section(160, 21.5, 79, 34.1, "PERFORM")
bw, bh = 17.4, 8.4
for i, (n, s) in enumerate((("BTN_BUILD", "BUILD"), ("BTN_DROP", "DROP"), ("BTN_BREAK", "BREAK"), ("BTN_FILL", "FILL"))):
    P.button(n, 162.5 + i * (bw + 1.6), 28.0, bw, bh, s, "#3a2a2e", "#ffb3b3", 1.6)
for i, (n, s) in enumerate((("BTN_NEWALL", "NEW ALL"), ("BTN_MUTATE", "MUTATE"), ("BTN_LOCKALL", "LOCK"), ("BTN_UNDO", "UNDO"))):
    P.button(n, 162.5 + i * (bw + 1.6), 38.4, bw, 7.0, s, "#ff8c2e" if s == "NEW ALL" else "#2c2e36", "#ffffff" if s == "NEW ALL" else TXT)
P.text(199.5, 49.6, "BUILD / DROP / BREAK: NEXT PHRASE", 1.15, DIM, 600, sp=0.2)
P.text(199.5, 52.8, "HOLD = STAY IN THE PART", 1.15, DIM, 600, sp=0.2)

# ---- the five tracks
TR = [("DRUMS", DR, "GROOVE"), ("BASS", BA, "ACID"), ("STAB", ME, "DUB"), ("LEAD", LE, "LOOP"), ("ATMO", PA, "MOVE")]
tw = (W - 10 - 4 * 2.0) / 5
ty = 59.0
for i, (name, col, fl) in enumerate(TR):
    x = 5 + i * (tw + 2.0)
    P.rect(x, ty, tw, 41.0, PLATE, 1.2)
    P.rect(x, ty, tw, 0.8, col, 0.3)
    P.text(x + tw / 2, ty + 4.6, name, 1.9, col, 800, sp=0.7)
    P.field(f"F_TYPE{i}", x + 2, ty + 6.6, tw - 4, 5.4, "TYPE", col)
    P.knob(f"K_DENS{i}", x + tw * 0.25, ty + 19.4, "large", col, "DENS")
    P.knob(f"K_FLAV{i}", x + tw * 0.585, ty + 19.4, "medium", None, fl, 3.3 + (12.19 - 9.6) / 2)
    P.knob(f"K_LEN{i}", x + tw * 0.86, ty + 19.4, "small", None, "LEN", 3.3 + (12.19 - 7.68) / 2)
    bw2 = (tw - 4 - 2 * 1.2) / 3
    for k, s in enumerate(("NEW", "LOCK", "MUTE")):
        P.button(f"BTN_{s}{i}", x + 2 + k * (bw2 + 1.2), ty + 32.0, bw2, 6.6, s, "#ff8c2e" if s == "NEW" else "#2c2e36", "#ffffff" if s == "NEW" else TXT, 1.35)

# ---- jacks
P.strip()
Y1, Y2 = 107.5, 121.0
x = 9.0
for k, (n, s) in enumerate((("IN_CLK", "CLK"), ("IN_RST", "RST"), ("IN_RUN", "RUN"), ("IN_NEW", "NEW"),
                            ("IN_BUILD", "BUILD"), ("IN_DROP", "DROP"), ("IN_ENERGY", "ENRG"), ("IN_EVOLVE", "EVOL"))):
    P.jack(n, x + k * J, Y1, s)
x = 9.0 + 8 * J + 3.0
P.rect(x - 4.3, 102.4, 8 * J - 0.6, 0.5, DR, 0.2)
for k, s in enumerate(("KICK", "SNR", "CLAP", "RIM", "CH", "OH", "TOM", "PERC")):
    P.jack(f"OUT_DRUM{k}", x + k * J, Y1, s, mix(DR), True)
x = W - 9.0 - 2 * J
for k, (n, s) in enumerate((("OUT_CLK", "CLK"), ("OUT_RST", "RST"), ("OUT_BAR", "BAR"))):
    P.jack(n, x + k * J, Y1, s, "#2c2e36", True)
x = 9.0
groups = [("BASS", ("V/OCT", "GATE", "VEL", "ACC", "SLIDE"), BA), ("STAB", ("V/OCT", "GATE", "VEL"), ME),
          ("LEAD", ("V/OCT", "GATE", "VEL", "ACC", "SLIDE"), LE), ("ATMO", ("V/OCT", "GATE", "MOD"), PA)]
for gname, labels, col in groups:
    P.rect(x - 4.3, 115.9, len(labels) * J - 0.6, 0.5, col, 0.2)
    for k, s in enumerate(labels):
        P.jack(f"OUT_{gname}{k}", x + k * J, Y2, s, mix(col), True)
    x += len(labels) * J + 1.6
x = W - 9.0 - J
for k, (n, s) in enumerate((("OUT_DUCK", "DUCK"), ("OUT_RISE", "RISE"))):
    P.jack(n, x + k * J, Y2, s, mix(RED), True)

P.finish(1800)
