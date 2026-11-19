# GENRA TECHNO – Merkliste (nur Offenes)

Stand 19. Nov. 2026, nach Build 2.0.7 (dev, kein Release).

## Zu testen in Rack
- 2.0.7: MIDI-MAP auf PLAY, STOP, NEW, LOCK, REC usw. lernen und drücken; MOD-PRESET mit einem Regler.
- 2.0.6: Familien BASS und GARAGE (12 Stile) nach Gehör, FEEL AUTO/FULL/HALF, STYLE-Liste mit Untermenüs, Wobble- und
  Reese-Bass mit GENRA-Bässen, Swing bis 66 %.
- 2.0.5: MOD-Tabelle mit Abschnitten als Spalten (Klick, Mausrad, Kopf blättert), IN / IN SEC / IN × SEC mit LFO,
  Abschnitts-Buchsen (Namen hell/grau, Tooltip), gestrichelte Linie im Display; ABLAUF-Zeitleiste (Klick, Rahmen),
  ROW CV 0–10 V über alle Zeilen, RST im Stillstand; MAIN/LIVE SONG unter PLAY; KEY/SCALE folgen dem Pattern;
  RUN als Gate; PATT-Änderungen aufs letzte Pad; LINK orange beim Rückfall; alte Patches laden.
- 2.0.4: ABLAUF-TRANSPORT (LOOP grau in PATT), Farben (PLAY grün, NEW orange, Modus blau), ein UNDO für Hauptmodul + GRID
  (auch KEY/STYLE), LOCK schützt vor jedem Neu-Erzeugen, Änderungen im SONG-Modus bleiben auf dem Pad, zweimal drücken
  bei GRID CLEAR / ABLAUF DELETE, Randomize, LEN auf 0, Reset.
- 2.0.3: ABLAUF TAKE LIVE (zweimal drücken): Pads pro Abschnitt, Zeilen wie im LIVE-Song, klingt gleich; UNDO; FORM-Feld lesbar.
- 2.0.2: SCALE-Dropdown (Wechsel behält Rhythmus), Follow mit GENRA/GENRA Instruments 2.40.0 (Bass, Melody, Pad, Drums, Stab), Presets folgen dem Stil.
- 2.0.1: MAIN-Loop beim Start, LIVE SONG, Klick in die Zeitleiste (Sprung am Taktende), < PART / PART >, kürzeres Intro.
- Hauptmodul allein: PLAY, LIVE-Song läuft durch (Intro bis Outro), Builds hörbar verschieden, PERFORM, FILL, MUTE/LOCK.
- GRID: Noten setzen/löschen/ziehen, END-Marke, Lanes, UNDO/REDO, REC über MIDI.
- ABLAUF: Pad speichern, Varianten, MAKE SONG, SONG + PLAY, LOOP, Export.
- MOD: Matrix, Song-View, Ausgänge an einem Filter; ohne Hauptmodul über CLK/PART.
- Schriftgrösse im Display bei 100 % Zoom (Entscheid nach dem Test).

## Für später (entschieden, noch nicht gebaut)

- **PATTERN statt MAIN, PLAY in PATT** (19. Nov. 2026):
  - Im Hauptmodul heisst MAIN neu **PATTERN**: spielt das Pattern von GENRA TECHNO als einfache Schleife (kein eigener
    Ablauf mit Intro/Build/Drop mehr; die gibt es von Hand über BUILD, DROP, BREAK).
  - Steht ABLAUF auf PATT und ist ein Pad gewählt, spielt dieses Pad in Schleife als sein eigener Teil (Break 2 als
    Break) – mit PLAY in ABLAUF und mit PLAY im Hauptmodul gleich. Ein anderes Pad am nächsten Takt, bis STOP.
  - PLAY in ABLAUF springt nicht mehr auf SONG, solange PATT gewählt ist. LIVE SONG (Hauptmodul) und SONG (ABLAUF,
    die Zeilen) bleiben die beiden Song-Arten.

- **MIDI-MAP für die übrigen Felder** (Prüfung 19. Nov. 2026, wie MOD-PRESET in 2.0.7 über `TField::mapSelect`):
  Hauptmodul STYLE, KEY, SCALE, FEEL, BUILD, TYPE ×5; ABLAUF SONG FORM; MOD TARGET ×4, BARS ×4. Bewusst nicht: Pads,
  Songliste, Zeitleisten, MOD-Tabelle, GRID-Editor.
- Weitere Module: GENRA TECHNO CHORD, danach GENRA TECHNO ACID.
- Look kann später noch geändert werden (vorerst dunkler GENRA-Look mit Spurfarben).
