# GENRA TECHNO – Übersicht (wo liegt was)

Plugin `GenraTechno` (Marke Genra), fünf Module. Stand 2.0.0 (16. Nov. 2026).

| Datei | Inhalt |
|---|---|
| `src/tg/Engine.hpp/.cpp` | Engine ohne Rack: Stile, Generator mit Bewertung, Songplan, Builds, GROW, `resolveStep`, MIDI |
| `src/TechnoModule.hpp`, `src/GenraTechno.cpp` | Hauptmodul: Player, LIVE-Song, PERFORM, Panel, Registry/Links |
| `src/TechnoGrid.cpp` | GRID (Editor, REC über MIDI) |
| `src/TechnoAblauf.cpp` | ABLAUF (Pads, Song-Zeilen, MAKE SONG, TAKE LIVE übernimmt den LIVE-Song, übergibt `SongScript` ans Hauptmodul) |
| `src/TechnoMod.cpp` | MOD (4 CV-Spuren, Matrix pro Teil, Song-View) |
| `src/TechnoKugel.cpp` | KUGEL (XY-Pad mit Ball und Physik, REC/LOOP im Takt; Follow GENRA direkt über den Link-Hub) |
| `src/TechnoUI.hpp` | Look: Farben, Taster, Dropdowns (Mausrad) |
| `src/link/` | Link-Hub der GENRA-Plugins (Kopie aus Genra-for-Vcv), für Follow |
| `tools/*_panel.py`, `tools/tpanel.py` | Panel-Skripte: schreiben `res/*.svg`, `src/*Layout.hpp`, `docs/panels/*.png` |
| `test/tg_test.cpp` | Engine-Test (läuft im Build vor dem Bauen) |

## Zusammenspiel

- GRID, ABLAUF und MOD finden das Hauptmodul über die Registry (`TechnoLink`, Rechtsklick › Connect to).
- Variante C: Das Hauptmodul allein spielt einen LIVE-Song. ABLAUF im SONG-Modus übergibt ein `SongScript`,
  dann spielt das Hauptmodul dessen Zeilen. MOD folgt immer dem, was das Hauptmodul gerade spielt.
- Follow (GENRA, GENRA Instruments ab 2.40.0): Das Hauptmodul meldet sich am Link-Hub an, Ausgänge im GENRA-Layout
  (DRUMS → Drums, BASS → Bass, LEAD → Melodie, ATMO → Pads) plus STAB (`genraports::STAB_*`, Spur „Stab“).
  `src/link/Follow.hpp` und `GenraPorts.hpp` sind Kopien aus Genra-for-Vcv und müssen gleich bleiben (Hub-Version).
  Presets der Folger wechselt `GenraTechno::rollPresets` (über den Hub, `link/GenreMatch.hpp` ist ebenfalls eine Kopie).
