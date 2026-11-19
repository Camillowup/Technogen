# GENRA TECHNO

A VCV Rack 2 plugin by Bastian Ofner, part of the GENRA family: a generative sequencer for techno and club music,
with its own grid editor, song arranger and song modulation. It only sends notes, gates and CV; the sounds come from
the instruments you patch (GENRA Instruments or any other module).

![GENRA TECHNO](docs/panels/genratechno.png)

## The modules

| Module | HP | What it does |
|---|---|---|
| **GENRA TECHNO** | 48 | The generator and player: 34 styles, five tracks, a whole song made live |
| **GENRA TECHNO GRID** | 44 | The editor: drum raster and piano roll, velocity, chance, ratchets, loop length per track |
| **GENRA TECHNO ABLAUF** | 34 | The song arranger: 64 pattern pads, variations per part, a song list, MAKE SONG |
| **GENRA TECHNO MOD** | 30 | Four CV lanes that move with the song (filter sweeps, sends, levels …) |
| **GENRA TECHNO KUGEL** | 24 | An XY pad with a ball that follows physics; record its way in sync and loop it |

GRID, ABLAUF and MOD connect to GENRA TECHNO without cables (automatically to the first one in the patch;
right-click › Connect to …).

## GENRA TECHNO

- **Styles** in ten families (the STYLE list has them as submenus; the field shows family and style): DETROIT (Classic, Soul), BERLIN (Hypnotic, Minimal, Dub Techno, Warehouse, Peak Time),
  ACID (Acid Techno, Acid House), HARD (Hard Techno, Industrial, Schranz), MELODIC (Melodic Techno, Progressive House),
  HOUSE (Tech House, Deep House, Chicago House), TRANCE (Trance, Progressive Trance, Psytrance), ELECTRO (Electro,
  Breaks), BASS (Dubstep, Riddim, Drum & Bass, Liquid DnB, Neurofunk, Jungle, Halftime), GARAGE (UK Garage, 2-Step,
  Speed Garage, Bassline, Future Garage). Every style has its own harmony: STATIC (one chord), MODAL (two chords) or PROGRESSION (a chord every bar).
- **KEY and SCALE**: SCALE is AUTO (the style's) or MINOR, DORIAN, PHRYGIAN, MAJOR, HARM. MINOR, MIXOLYDIAN, LYDIAN; a new
  scale keeps the rhythm, every note keeps its degree (locked tracks stay).
- **FEEL**: AUTO (the style's), FULL (backbeat on 2 and 4) or HALF (on 3, half-time); a new feel makes new drums
  (not when DRUMS is locked). New drum types HALF-TIME, DNB TWO-STEP, AMEN CHOPS, UK 2-STEP; bass types WOBBLE, REESE,
  BASSLINE. SWING goes up to 66 % (garage).
- **Five tracks**: DRUMS (8 voices in GENRA's order: kick, snare, clap, rim, closed hat, open hat, tom, perc), BASS,
  STAB (poly 4), LEAD and ATMO (poly 4). Per track: a TYPE (e.g. ACID 303, DUB CHORD, HYPNO LOOP), DENS, a flavour
  knob (GROOVE, ACID, DUB, LOOP, MOVE), LEN (the track's own loop length in 16ths: polymeter), NEW, LOCK and MUTE.
- **Better patterns**: for every track the generator makes several candidates and keeps the one the rules like best
  (kick and bass apart, enough repetition, chord tones on the strong beats …).
- **PATTERN** (was MAIN; the default, and after NEW ALL): the pattern in a loop, all tracks, 8 bars again and again (as a
  drop, or as the part of the ABLAUF pad picked in PATT: BREAK 2 as a break). MUTATE and EVOLVE change it; BUILD, DROP,
  BREAK play their part for 8 bars over it. Work on the theme here (NEW, LOCK, TYPE, DENS …), then switch to the song.
- **LIVE SONG**: without ABLAUF (or with ABLAUF in PATT mode) GENRA TECHNO makes the whole song itself: intro, groove,
  build, drop, break, build, drop, outro, as long as LENGTH says (2–12 minutes). One element changes every 8 bars.
  Right-click › DJ intro and outro: 32 bars, drums only (else a 16-bar intro).
- **The timeline** in the display: click a part (or a place in it) and the song goes there at the end of the bar;
  < PART and PART > step through the parts.
- **Builds** (BUILD: AUTO picks what the style likes, or choose one): snare roll, kick, hat roll, arp, stutter, strip,
  poly, cut, none, fake. The second build and drop are bigger (GROW: longer, faster rolls, a moment of silence before
  the drop, more percussion, the lead an octave up in melodic styles).
- **PERFORM**: BUILD, DROP and BREAK go there at the start of the next phrase (8 bars); hold the key to stay in that
  part. FILL plays a fill at the end of the bar. NEW ALL, MUTATE, LOCK (all tracks), UNDO.
- **LOCK** keeps a track as it is: NEW, NEW ALL, NEW TRACK, the NEW input, MUTATE, EVOLVE, a new TYPE and ABLAUF's
  variations leave it alone (a TYPE chosen while locked comes with the next NEW). Edits by hand and KEY still act.
- **UNDO** (here and in GRID, one memory for both) also brings back style, key, scale, types, tempo and swing.
- **Colours of the keys** (all four modules): orange makes something new, green plays, blue is the chosen mode.
- **ENERGY** thins out or fills up hats, ghosts and percussion; **EVOLVE** changes one track a little every 8 bars.
- **Outputs**: 8 drum triggers (level = velocity), BASS and LEAD (V/OCT, GATE, VEL, ACC, SLIDE), STAB (V/OCT and
  GATE poly 4, VEL), ATMO (V/OCT and GATE poly 4, MOD), CLK, RST, BAR, DUCK (a sidechain envelope on every kick) and
  RISE (0–10 V over a build).
- **Inputs**: CLK (16ths), RST, RUN, NEW, BUILD, DROP, ENERGY and EVOLVE CV.
- Right-click: export the pattern or the LIVE song as MIDI (into the MIDI folder of the GENRA plugins).
- **Follow GENRA**: the instruments of GENRA and GENRA Instruments (2.40.0 or newer) follow GENRA TECHNO without cables
  (right-click on the instrument › Follow GENRA): Drums, Bass, Melody (= LEAD), Pad (= ATMO) or Stab (GENRA TECHNO).
  With "Presets follow GENRA" on, NEW and a new style load a preset for the style.

## GENRA TECHNO GRID

Pick a track (DRUMS … ATMO). Click: add or select a note; right-click: delete; drag a note: its length; click in the
ruler: the END of the track's loop. Under the roll: velocity and chance (drag), ratchets (click: 1–4). VIEW: BAR,
ZOOM (1/2/4 bars), PITCH, ALL · SCALE · USED. NOTES: VEL, LEN, CHANCE, RATCH, ACCENT, SLIDE, SNAP (only scale notes),
FOLLOW (the page follows the playhead). EDIT: UNDO, REDO, CLEAR (press twice), COPY, PASTE, NEW TRACK. SOLO plays only this track.
REC writes MIDI notes at the playing step (right-click › MIDI input).

## GENRA TECHNO ABLAUF

- **PATTERNS**: tap an empty pad to store what GENRA TECHNO plays; tap a pad to load it (PATT mode). VARIATION
  INTRO … OUTRO makes a variant of the last pattern for that part on the next free pad (press again: another one).
  COPY (from, to), CLEAR (then a pad), CLEAR ALL (press twice; UNDO brings it back). Right-click a pad: MIDI export.
- **SONG**: the rows (pad, bars, part, build). ROW picks a row, PAD, BARS and PART change it; INSERT puts the last pad
  after it, DELETE (press twice), UNDO. FORM + MAKE SONG builds a song in a club form from the pads.
  TAKE LIVE (press twice) takes GENRA TECHNO's LIVE song: one pad per section with the pattern it plays, the rows as
  in the LIVE song (UNDO: back).
  DJ IN/OUT: intro and outro drums only.
- **TRANSPORT** (beside RUN, RST, NEXT): PATT or SONG, PLAY plays the song from the selected row (SONG mode; green
  while it plays), STOP stops, LOOP repeats the selected row (only in SONG mode, else greyed out).
- While the song plays, edits in GENRA TECHNO and GRID (NEW, KEY, notes …) go onto the pad that plays, so they are
  there again the next time the song comes to it.
  EXPORT SONG / EXPORT PAD write MIDI.
- Inputs: RUN, RST, NEXT, PAD CV, ROW CV. Outputs: END, ROW, PART (0–5 V), BUILD (gate), DROP, PHRASE.

## GENRA TECHNO MOD

Four lanes (A–D). The table MOVE PER SECTION has the song's sections as columns (9 shown; the mouse wheel on the head
pages) and the lanes as rows. A cell takes its part's movement unless it is set for that one section (a white dot).
Movements: OFF, LOW, MID, HIGH, RISE, FALL, SWELL, FADE, WAVE, PUMP, WANDER, STEPS, JUMP, DIP and from outside IN (the
lane's jack IN A–D), IN SEC (the section's jack) and IN × SEC (both multiplied). A click opens the list (with "Like every
…" and "This for every …"); the mouse wheel steps through it. Below are ten section jacks: INTRO, GROOVE 1/2, BUILD 1/2,
DROP 1/2, BREAK 1/2, OUTRO (a third section of a part uses its "2"). Their names are bright in the part's colour when the
song has the section, grey and struck out when not (a cable there does nothing); the tooltip says the row and bars. The
display draws what comes in from outside as a line. Per lane: DEPTH, SMOOTH, TARGET, BARS, UNI (0–10 V) / BI (±5 V).
GROW: later drops bigger; CHORD: a little lift on every chord change; FREEZE: hold. Presets in the head. Outputs A–D,
PHRASE, PART OUT and BAR on the right. Without a GENRA TECHNO the table has the six parts.

MOD follows the song GENRA TECHNO plays: ABLAUF's when ABLAUF is in SONG mode, else the LIVE song. Without a GENRA
TECHNO it counts its CLK input (16ths) and takes the part from PART (0–5 V). The display shows the whole song, the
four lanes over it and where the song is.

## GENRA TECHNO KUGEL

An XY pad with a ball, after the Kaoss Physics of the Korg modwave. Drag the ball with the mouse and let go to throw it.

- **PHYSICS:** GRAVITY (0–2 g), TILT (an endless knob: the direction gravity pulls, 0° is down, all 360°), FRICTION,
  MASS (heavier: kicks and pushes move it less), BOUNCE (how much speed it keeps at a wall).
- **WALL:** BOUNCE (it bounces off), WRAP (out on one side, in on the other), STICK (it stays at the wall until a kick),
  SLIDE (it slides along the wall), ROUND (a round arena).
- **RANGE** of X, Y and SPEED: −10…0 V, ±5 V or 0…10 V. SMOOTH glides them. HIT: a trigger at every wall.
- **KICK** throws the ball in a random direction (FORCE: how hard); a GATE does the same, VEL sets the strength.
  **HOLD** holds it where it is. PUSH X / PUSH Y (±5 V) push it.
- **REC** records the ball's way: it starts on the next bar and runs LOOP LEN bars (1, 2, 4 or 8), then **LOOP** plays it
  back in sync (press LOOP to go back to the free ball, again to play the loop). RESET starts the loop again at bar 1.
  The REC jack works like the key; right-click on REC: clear the loop.
- **Time:** from the CLK jack (16ths; right-click › Clock input: 8ths, quarters or 24 PPQN), else from the GENRA it
  follows (right-click › Follow GENRA: GENRA, GENRA TECHNO or GENRA CHOP, automatically the first one; LINK lights up),
  else its own clock at 120 BPM. Following a GENRA, the ball is also kicked by one of its tracks (right-click › Kick
  from: drums (the kick), bass, melody, pads, stab or nothing). A cable in CLK or GATE always wins.
- Presets in the head (PINBALL, SUPERBALL, MOON, BILLIARD, RAIN, ORBIT …). MIDI-MAP can turn every knob and field,
  press the four keys and go through the presets.

## Building

Windows and Mac builds come from GitHub Actions (Actions › the latest run › Artifacts). By hand:
`make RACK_DIR=/path/to/Rack-SDK dist`. The engine has its own test: `g++ -std=c++11 test/tg_test.cpp src/tg/Engine.cpp`.

## License

GPL-3.0-only. `src/link/` and `src/MidiFolder.*` are copied from the GENRA plugin (same author, same license).
