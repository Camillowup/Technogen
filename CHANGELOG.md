# Changelog

## 2.0.9 (19 Nov 2026, test build, no release)

- New module **GENRA TECHNO KUGEL** (24 HP): an XY pad with a ball that follows physics (after the modwave's Kaoss
  Physics). GRAVITY, TILT (endless, 360°), FRICTION, MASS, BOUNCE; WALL BOUNCE / WRAP / STICK / SLIDE / ROUND; RANGE
  −10…0 V, ±5 V, 0…10 V; KICK, HOLD, GATE / VEL, PUSH X / Y; outputs X, Y, SPEED, HIT. REC records the ball's way on the
  next bar for 1, 2, 4 or 8 bars, LOOP plays it back in sync. Time from CLK (resolution by right-click), from the GENRA it
  follows (Follow GENRA through the link hub) or its own 120 BPM. 20 presets; MIDI-MAP on everything.

## 2.0.8 (19 Nov 2026, test build, no release)

- MAIN is now **PATTERN**: the pattern in a loop (8 bars again and again; MUTATE and EVOLVE change it). BUILD, DROP and
  BREAK play their part for 8 bars over it, then the loop goes on (they no longer leave it for the song).
- ABLAUF in PATT: a pad loops as its own part (BREAK 2 as a break), with PLAY in ABLAUF and PLAY on GENRA TECHNO alike;
  another pad from the next bar. PLAY no longer jumps to SONG while PATT is chosen; in SONG it plays the rows.
- MIDI-MAP turns the fields too: STYLE, KEY, SCALE, FEEL, BUILD and the five TYPEs, ABLAUF's SONG FORM, MOD's TARGET and
  BARS (the field shows the entry at once and takes it when the knob rests for 0.3 s).

## 2.0.7 (19 Nov 2026, test build, no release)

- MIDI-MAP can press every button of the four modules (not the pads): PLAY, STOP, MAIN, LIVE SONG, NEW and LOCK per
  track, NEW ALL, LOCK ALL, MUTE, BUILD, DROP, BREAK, FILL, MUTATE, PREV, NEXT, UNDO; in ABLAUF PLAY, STOP, LOOP, PATT,
  SONG, TAKE LIVE, MAKE SONG, DJ, BANK, VARIATION, INSERT, DELETE, COPY, CLEAR, UNDO; in GRID REC, the tracks, SOLO, VIEW,
  ACCENT, SLIDE, SNAP, FOLLOW, UNDO, REDO, CLEAR, COPY, PASTE, NEW; in MOD GROW, CHORD, FREEZE, UNI/BI. A value over half
  presses the button once, as a click does.
- MOD: the PRESET field can be learnt by MIDI-MAP; a knob goes through the presets (it chooses when the knob rests).

## 2.0.6 (18 Nov 2026, test build, no release)

- Two new families with 12 styles: BASS (DUBSTEP, RIDDIM, DRUM & BASS, LIQUID DNB, NEUROFUNK, JUNGLE, HALFTIME) and
  GARAGE (UK GARAGE, 2-STEP, SPEED GARAGE, BASSLINE, FUTURE GARAGE). Bass music gets to its first drop early.
- New field **FEEL** under the display: AUTO (the style's), FULL or HALF (the backbeat on 3). A new feel makes new drums.
- New drum types HALF-TIME, DNB TWO-STEP, AMEN CHOPS, UK 2-STEP and bass types WOBBLE, REESE, BASSLINE.
- The FAM field is gone: STYLE shows family and style, its list has the families as submenus (with the tempo).
- SWING up to 66 %.

## 2.0.5 (18 Nov 2026, test build, no release)

- MOD: the table is turned. Columns are the song's sections (from ABLAUF or the LIVE song), rows the lanes A–D. A cell
  takes its part's movement unless set for that one section (marked). New movements IN, IN SEC and IN × SEC; jacks IN A–D
  beside the rows and ten section jacks (INTRO, GROOVE 1/2, BUILD 1/2, DROP 1/2, BREAK 1/2, OUTRO), named bright when
  the song has the section, grey and struck out when not. Click: the list; wheel: next; wheel on the head: more sections.
  The display draws what comes in from outside. Outputs on the right, PART is now PART OUT.
- ABLAUF: a timeline of the song over the whole width; the selected row is framed, a click picks a row. ROW CV 0–10 V
  spans all the rows (shown by the frame). RST while stopped goes back to the first row.
- GENRA TECHNO: MAIN and LIVE SONG under STOP and PLAY. KEY and SCALE show the pattern that plays.
- RUN can be a gate (right-click), on GENRA TECHNO and ABLAUF.
- A pattern sent while one pad plays does not land on the next one. In PATT mode edits go onto the pad loaded last.
- A link to a GENRA TECHNO that is gone falls back to the first one; the LINK light is orange then.
- Patterns from older (smaller) saves load instead of being dropped.

## 2.0.4 (18 Nov 2026, test build, no release)

- ABLAUF: a **TRANSPORT** section beside the jacks RUN, RST and NEXT: PATT / SONG, PLAY, STOP, LOOP. The SONG section
  keeps the rows: ROW, PAD, BARS, PART over the whole width, INSERT, DELETE, UNDO. LOOP is greyed out in PATT mode.
- Colours of the keys in all four modules: PLAY is dark and green while it plays; NEW, NEW ALL, NEW TRACK, MAKE SONG
  and TAKE LIVE are orange; the chosen mode (MAIN / LIVE SONG, PATT / SONG) is blue; DROP looks like BUILD and BREAK.
- One UNDO for GENRA TECHNO and GRID (REDO in GRID); it brings back style, key, scale, types, tempo and swing too.
- LOCK: a locked track is never made new (NEW, NEW TRACK, the NEW input, a new TYPE, ABLAUF's variations as well).
- While ABLAUF's song plays, edits by hand go onto the pad that plays (they were lost at the next row).
- ABLAUF: PAD, BARS and PART show the selected row after MAKE SONG, TAKE LIVE, DELETE and UNDO; ROW turns only over
  the rows there are; UNDO also takes the knobs PAD / BARS / PART and the right-click Build and GROW.
- Press twice: GRID CLEAR and ABLAUF DELETE (like CLEAR ALL and TAKE LIVE).
- Randomize leaves tempo, song length, the LEN knobs, ABLAUF's knobs and GRID's note knobs alone.
- LEN back on 0 gives the track its length from before at once. Reset also resets SCALE and the DJ intro.

## 2.0.3 (18 Nov 2026, test build, no release)

- ABLAUF: new key **TAKE LIVE** beside MAKE SONG. Press twice (SURE?): ABLAUF takes GENRA TECHNO's LIVE song as it is
  planned now. The pads are cleared, every section gets a pad of its own with the pattern GENRA TECHNO plays (INTRO 1,
  GROOVE 1, BUILD 1, DROP 1, … DROP 2 …), the song rows are the sections as they are (bars, part, build, the bigger later
  drops), DJ IN/OUT as in the LIVE song, and ABLAUF goes to SONG. It sounds the same; now every section can be changed on
  its own pad. UNDO brings back what was there. (EVOLVE and PERFORM jumps are played live and are not taken over.)
- The FORM field is narrower: it shows the form's name, the list its length too.

## 2.0.2 (16 Nov 2026, test build, no release)

- The choices under the display are narrower: FAM, STYLE, KEY, the new **SCALE** and BUILD. SCALE: AUTO (the style's),
  MINOR, DORIAN, PHRYGIAN, MAJOR, HARM. MINOR, MIXOLYDIAN, LYDIAN. A new scale keeps rhythm and key: every note keeps its
  degree, locked tracks stay as they are. GRID's SNAP and SCALE view use it too.
- **Follow GENRA**: the instruments of GENRA and GENRA Instruments (from their 2.40.0) can follow GENRA TECHNO without
  cables: DRUMS, BASS, LEAD (as melody), ATMO (as pads) and, new, **Stab (GENRA TECHNO)**. NEW ALL, NEW of a track and
  a new style load a preset for the style in the instruments that follow (Presets follow GENRA).

## 2.0.1 (16 Nov 2026, test build, no release)

- MAIN | LIVE SONG in the display: MAIN (the default, also after NEW ALL) plays the drop in a loop, all tracks, so you
  hear the theme first; LIVE SONG plays the whole song.
- The timeline in the display is a scroll bar: click a part (or a place in it) and the song goes there at the end of
  the bar (to the start of that 8-bar phrase). < PART and PART > step a part back or on.
- Shorter LIVE intro (16 bars) and the elements come in faster; the long DJ intro and outro (32 bars, drums only) are
  a right-click option now. ABLAUF's forms with DJ IN/OUT keep the long ones.

## 2.0.0 (16 Nov 2026, test build, no release)

- First version: GENRA TECHNO, GENRA TECHNO GRID, GENRA TECHNO ABLAUF, GENRA TECHNO MOD.
