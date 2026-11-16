// SPDX-License-Identifier: GPL-3.0-only
// GENRA TECHNO: the engine (no Rack in here, so it can be tested on its own: test/tg_test.cpp).
//
// A pattern is up to 4 bars of 16ths for five tracks: DRUMS (8 voices in GENRA's order), BASS, STAB (poly 4), LEAD and
// ATMO (poly 4). Every track (and every drum voice) loops over its own length, so a 7-step bass runs against the
// 16-step kick (polymeter).
//
// generate() writes a pattern for a style: for every track it makes a handful of candidates and keeps the one the rules
// like best (kick and bass apart, the density the style wants, enough repetition, chord tones on the strong beats …).
//
// A song is a list of sections (INTRO, GROOVE, BUILD, DROP, BREAK, OUTRO) with their length in bars. planSong() makes
// one for a style and a length in minutes (LIVE mode); ABLAUF can hand over its own list (SONG mode). resolveStep()
// turns "this pattern, this section, this bar, this 16th" into what plays: which elements are in (one change every
// 8 bars, intro and outro DJ-friendly), the build of the section (snare roll, kick build, hat roll, arp speed-up,
// stutter, strip, polyrhythm, cut, none, fake), fills at the end of a phrase, and GROW (a later build and drop bigger).
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace tg {

enum { STEPS = 64, BAR = 16, DRUMS = 8, MELS = 4, VOICES = 4 };
enum Track { T_DRUMS, T_BASS, T_STAB, T_LEAD, T_ATMO, NUM_TRACKS };
enum Drum { D_KICK, D_SNARE, D_CLAP, D_RIM, D_CH, D_OH, D_TOM, D_PERC };
extern const char* const TRACK_NAMES[NUM_TRACKS];
extern const char* const DRUM_NAMES[DRUMS];
extern const char* const DRUM_SHORT[DRUMS];

// ------------------------------------------------------------------ the pattern
struct Hit {
	uint8_t vel = 0;      // 0: no hit, else 1..127
	uint8_t chance = 100; // %
	uint8_t ratch = 1;    // 1..4 hits in the 16th
	int8_t micro = 0;     // timing in 1/100 of a 16th (-40..40)
};
struct Note {
	uint8_t on = 0;
	uint8_t vel = 100;
	uint8_t len = 1; // in 16ths (1..64)
	uint8_t chance = 100;
	uint8_t ratch = 1;
	uint8_t acc = 0;
	uint8_t slide = 0;
	uint8_t n = 1;               // voices used (STAB, ATMO: up to 4)
	int8_t pitch[VOICES] = {48, 0, 0, 0}; // MIDI notes
	int8_t micro = 0;
};
struct Pattern {
	Hit drum[DRUMS][STEPS];
	Note mel[MELS][STEPS]; // BASS, STAB, LEAD, ATMO
	uint8_t len[NUM_TRACKS];  // loop length per track in 16ths (1..64)
	uint8_t vlen[DRUMS];      // loop length per drum voice (0: the track's)
	uint8_t bars = 4;         // the pattern's own length (1..4); the display and MIDI export use it
	uint8_t style = 2;
	uint8_t key = 9;          // 0 C … 9 A …
	uint8_t mode = 0;         // scale (SCALES)
	int8_t chord[4][VOICES];  // one chord per bar (pitches around middle C, for display, STAB and ATMO)
	uint8_t chordN[4];
	uint8_t chordDeg[4];      // scale degree of the chord root (0..6)
	uint32_t seed = 1;
	uint8_t version = 1;
	Pattern();
	int trackLen(int t) const { return len[t] ? len[t] : 16; }
	int voiceLen(int v) const { return vlen[v] ? vlen[v] : trackLen(T_DRUMS); }
	void clearTrack(int t);
};

// ------------------------------------------------------------------ styles
enum Family { F_DETROIT, F_BERLIN, F_ACID, F_HARD, F_MELODIC, F_HOUSE, F_TRANCE, F_ELECTRO, NUM_FAMILIES };
extern const char* const FAMILY_NAMES[NUM_FAMILIES];
enum Harmony { H_STATIC, H_MODAL, H_PROG };
extern const char* const HARMONY_NAMES[3];
enum KickType { K_FOUR, K_FOUR_GHOST, K_BROKEN, K_BREAKS };
enum BassType { B_OFFBEAT, B_ROLLING, B_ACID, B_FUNK, B_LONG, B_GALLOP };
enum StabType { S_NONE, S_DUB, S_CHORD, S_ONE, S_PLUCK };
enum LeadType { L_NONE, L_LOOP, L_ARP, L_MOTIF, L_ACID };
enum AtmoType { A_DRONE, A_PAD, A_HIGH };
enum BuildType { B_AUTO, BT_SNARE, BT_KICK, BT_HATS, BT_ARP, BT_STUTTER, BT_STRIP, BT_POLY, BT_CUT, BT_NONE, BT_FAKE, NUM_BUILDS };
extern const char* const BUILD_NAMES[NUM_BUILDS];

struct Style {
	const char* name;
	int family;
	float bpm;
	float swing;  // 50 straight … 58
	int mode;     // SCALES
	int harmony;
	int kick, bass, stab, lead, atmo;
	float hats;   // 0 sparse … 1 busy
	float perc;   // 0 … 1
	bool clap;    // clap on 2 and 4 (else snare / sparse)
	bool rich;    // 7ths and 9ths in the chords
	int builds[4]; // the builds the style likes (first = favourite)
	bool fullHat; // closed hats in 16ths (else 8ths)
	int stabLen;  // stab loop length (16 or an odd one)
	int leadLen;
};
extern const std::vector<Style>& styles();
int numStyles();
/** Styles of a family, in list order. */
std::vector<int> stylesOf(int family);

struct Scale {
	const char* name;
	int n;
	int iv[7];
};
extern const Scale SCALES[5];
extern const char* const KEY_NAMES[12];

// ------------------------------------------------------------------ generating
struct GenSettings {
	int style = 2;
	int key = 9;
	float energy = 0.6f; // 0..1
	float dens[NUM_TRACKS] = {0.5f, 0.5f, 0.5f, 0.5f, 0.5f};
	float flavour[NUM_TRACKS] = {0.5f, 0.5f, 0.5f, 0.5f, 0.5f}; // GROOVE, ACID, DUB, LOOP, MOVE knobs
	int len[NUM_TRACKS] = {0, 0, 0, 0, 0};                         // loop length in 16ths, 0: the generator's
	int type[NUM_TRACKS] = {-1, -1, -1, -1, -1};                  // per-track type override (-1: style's)
};
/** A whole new pattern (track -1) or one track (the others stay). */
void generate(Pattern& p, const GenSettings& s, uint32_t seed, int track = -1);
/** Small change of one track (EVOLVE / MUTATE): a few hits or notes move. amount 0..1. */
void mutate(Pattern& p, const GenSettings& s, uint32_t seed, int track, float amount);
/** Names of the per-track type lists (the dropdown in each track). */
int typeCount(int track);
const char* typeName(int track, int i);
/** The type the style uses for a track (index into typeName). */
int styleType(int style, int track);
/** The pattern's chords and scale again (after the key or style changed). */
void makeChords(Pattern& p, int style, int key, uint32_t seed);
std::string chordName(const Pattern& p, int bar);
bool inScale(const Pattern& p, int midi);

// ------------------------------------------------------------------ the song
enum Part { P_INTRO, P_GROOVE, P_BUILD, P_DROP, P_BREAK, P_OUTRO, NUM_PARTS };
extern const char* const PART_NAMES[NUM_PARTS];
struct Section {
	uint8_t part = P_GROOVE;
	uint8_t bars = 16;
	uint8_t build = BT_NONE; // the build of a BUILD section
	uint8_t grow = 0;        // 0 normal, 1 the second, 2 the third … (GROW: bigger)
	uint8_t dropIdx = 0;     // DROP n (0 first)
	uint8_t pad = 0;         // ABLAUF: the pad it plays
};
/** A song for a style: minutes long (at bpm), with the builds the style likes (forced: one build type for all).
 * djIntro: a 32-bar intro (else 16). */
std::vector<Section> planSong(int style, float minutes, float bpm, uint32_t seed, int forcedBuild = B_AUTO, bool grow = true,
                              bool djIntro = false);
int songBars(const std::vector<Section>& s);
/** Choose the build type for a section of a style (avoid: the one before). */
int pickBuild(int style, uint32_t seed, int avoid, int grow);

// ------------------------------------------------------------------ playing
struct Ctx {
	int part = P_GROOVE;
	int secBar = 0;   // bar within the section
	int secBars = 16;
	int build = BT_NONE;
	int grow = 0;
	int dropIdx = 0;
	int nextPart = P_GROOVE; // what comes after this section
	bool djEdges = true;     // intro and outro: drums only
	bool fill = false;       // a fill in this bar (FILL pressed)
	float energy = 0.6f;
	int64_t songStep = 0;    // 16ths since the start (loops the tracks)
	uint32_t rnd = 1;        // for the chance of each hit
};
struct DrumFire {
	uint8_t vel = 0;
	uint8_t ratch = 1;
	int8_t micro = 0;
};
struct NoteFire {
	bool on = false;
	Note note;
	int octave = 0; // GROW: the lead an octave up
};
struct StepOut {
	DrumFire drum[DRUMS];
	NoteFire mel[MELS];
	float rise = 0.f; // 0..1 during a build
	bool elementOn[NUM_TRACKS + DRUMS] = {};
};
/** What plays in this 16th. */
void resolveStep(const Pattern& p, const Ctx& c, StepOut& out);
/** Is the element in at this point of the song (track; for drums the voice too)? */
bool elementIn(int style, const Ctx& c, int track, int voice);

// ------------------------------------------------------------------ MIDI
struct MidiEv {
	int64_t tick;
	uint8_t status, d1, d2;
};
/** A standard MIDI file (type 1): tempo, then one track per element (drums on channel 10, GM notes). */
std::vector<uint8_t> writeMidi(const std::vector<std::vector<MidiEv>>& tracks, const std::vector<std::string>& names,
                               float bpm, int ppq);
/** The whole song (or just the pattern for `bars` bars when song is empty) as MIDI. */
std::vector<uint8_t> songMidi(const std::vector<const Pattern*>& pats, const std::vector<Section>& song, float bpm,
                              float energy, bool djEdges, int onlyTrack = -1);
extern const uint8_t GM_DRUM[DRUMS];

// ------------------------------------------------------------------ small things
struct Rng {
	uint32_t s;
	explicit Rng(uint32_t seed) : s(seed ? seed : 0x9e3779b9u) {}
	uint32_t next() {
		s ^= s << 13;
		s ^= s >> 17;
		s ^= s << 5;
		return s;
	}
	float uni() { return (next() >> 8) * (1.f / 16777216.f); }
	int range(int n) { return n <= 1 ? 0 : (int) (next() % (uint32_t) n); }
	bool chance(float p) { return uni() < p; }
};
uint32_t hash(uint32_t a, uint32_t b = 0, uint32_t c = 0);

} // namespace tg
