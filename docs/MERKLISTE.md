# GENRA TECHNO – Merkliste (nur Offenes)

Stand 19. Nov. 2026, nach Build 2.0.9 (dev, kein Release).

## Zu testen in Rack
- 2.0.9 KUGEL: Ball werfen (ziehen und loslassen), GRAVITY/TILT (Endlos-Regler, Pfeil im Pad), FRICTION, MASS, BOUNCE,
  die fünf WALL-Arten, RANGE an X/Y/SPEED, SMOOTH, HIT; KICK, HOLD, GATE/VEL, PUSH X/Y; REC startet am nächsten Takt,
  LOOP spielt synchron (mit GENRA und GENRA TECHNO per Follow, mit CLK in 16tel/8tel/Vierteln/24 PPQN, ohne beides
  120 BPM), RESET setzt auf Takt 1; Presets; MIDI-MAP auf Regler, Felder, Tasten, Preset; Loop bleibt im Patch gespeichert.
- 2.0.8: PATTERN (statt MAIN) als Schleife; ABLAUF auf PATT: Pad antippen, PLAY in ABLAUF oder im Hauptmodul – das Pad
  läuft als sein Teil (BREAK 2 als Break), anderes Pad am nächsten Takt; BUILD/DROP/BREAK 8 Takte über der Schleife;
  Felder (STYLE, KEY, SCALE, FEEL, BUILD, TYPE, SONG FORM, MOD TARGET/BARS) per MIDI-MAP.
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

- KUGEL: versteckte Parameter BALL X / BALL Y, damit man den Ball mit einem XY-Pad oder Joystick am Controller führen
  kann (beim Loslassen fliegt er mit Schwung weiter).

- Weitere Module: GENRA TECHNO CHORD, danach GENRA TECHNO ACID.
- Look kann später noch geändert werden (vorerst dunkler GENRA-Look mit Spurfarben).
