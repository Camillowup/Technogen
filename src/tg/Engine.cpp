// SPDX-License-Identifier: GPL-3.0-only
// GENRA TECHNO: the engine (see Engine.hpp).
#include "Engine.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace tg {

const char* const TRACK_NAMES[NUM_TRACKS] = {"DRUMS", "BASS", "STAB", "LEAD", "ATMO"};
const char* const DRUM_NAMES[DRUMS] = {"Kick", "Snare", "Clap", "Rim", "Closed hat", "Open hat", "Tom", "Perc"};
const char* const DRUM_SHORT[DRUMS] = {"KICK", "SNR", "CLAP", "RIM", "CH", "OH", "TOM", "PERC"};
const char* const FAMILY_NAMES[NUM_FAMILIES] = {"DETROIT", "BERLIN", "ACID", "HARD", "MELODIC", "HOUSE", "TRANCE", "ELECTRO"};
const char* const HARMONY_NAMES[3] = {"STATIC", "MODAL", "PROGRESSION"};
const char* const BUILD_NAMES[NUM_BUILDS] = {"AUTO",  "SNARE ROLL", "KICK",  "HAT ROLL", "ARP",  "STUTTER",
                                             "STRIP", "POLY",       "CUT",   "NONE",     "FAKE"};
const char* const PART_NAMES[NUM_PARTS] = {"INTRO", "GROOVE", "BUILD", "DROP", "BREAK", "OUTRO"};
const char* const KEY_NAMES[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
const Scale SCALES[5] = {{"MINOR", 7, {0, 2, 3, 5, 7, 8, 10}},
                         {"DORIAN", 7, {0, 2, 3, 5, 7, 9, 10}},
                         {"PHRYGIAN", 7, {0, 1, 3, 5, 7, 8, 10}},
                         {"MAJOR", 7, {0, 2, 4, 5, 7, 9, 11}},
                         {"HARM. MINOR", 7, {0, 2, 3, 5, 7, 8, 11}}};
const uint8_t GM_DRUM[DRUMS] = {36, 38, 39, 37, 42, 46, 45, 56};

uint32_t hash(uint32_t a, uint32_t b, uint32_t c) {
	uint32_t h = 2166136261u;
	uint32_t v[3] = {a, b, c};
	for (int i = 0; i < 3; i++) {
		h ^= v[i] + 0x9e3779b9u + (h << 6) + (h >> 2);
		h *= 16777619u;
	}
	h ^= h >> 15;
	h *= 0x2c1b3c6du;
	h ^= h >> 12;
	return h ? h : 1;
}

Pattern::Pattern() {
	for (int t = 0; t < NUM_TRACKS; t++)
		len[t] = 16;
	for (int v = 0; v < DRUMS; v++)
		vlen[v] = 0;
	std::memset(chord, 0, sizeof chord);
	for (int b = 0; b < 4; b++) {
		chordN[b] = 0;
		chordDeg[b] = 0;
	}
}

void Pattern::clearTrack(int t) {
	if (t == T_DRUMS) {
		for (int v = 0; v < DRUMS; v++)
			for (int s = 0; s < STEPS; s++)
				drum[v][s] = Hit();
	}
	else if (t > 0 && t < NUM_TRACKS)
		for (int s = 0; s < STEPS; s++)
			mel[t - 1][s] = Note();
}

// ------------------------------------------------------------------ styles
//  name                family     bpm  swing mode harmony   kick          bass       stab     lead     atmo    hats perc clap  rich  builds                                    16th  stabL leadL
static const std::vector<Style> STYLES = {
    {"DETROIT CLASSIC", F_DETROIT, 128, 53, 1, H_PROG, K_FOUR, B_FUNK, S_CHORD, L_MOTIF, A_PAD, .7f, .5f, true, true, {BT_NONE, BT_HATS, BT_STRIP, BT_SNARE}, true, 16, 32},
    {"DETROIT SOUL", F_DETROIT, 124, 54, 1, H_PROG, K_FOUR, B_FUNK, S_CHORD, L_ARP, A_PAD, .6f, .6f, true, true, {BT_STRIP, BT_NONE, BT_HATS, BT_ARP}, true, 16, 16},
    {"HYPNOTIC", F_BERLIN, 132, 51, 0, H_STATIC, K_FOUR, B_ROLLING, S_ONE, L_LOOP, A_DRONE, .8f, .6f, false, false, {BT_NONE, BT_STRIP, BT_POLY, BT_KICK}, true, 24, 12},
    {"MINIMAL", F_BERLIN, 126, 54, 0, H_STATIC, K_FOUR_GHOST, B_OFFBEAT, S_ONE, L_LOOP, A_HIGH, .5f, .7f, true, false, {BT_STRIP, BT_NONE, BT_CUT, BT_POLY}, false, 32, 10},
    {"DUB TECHNO", F_BERLIN, 122, 52, 0, H_MODAL, K_FOUR, B_LONG, S_DUB, L_NONE, A_HIGH, .4f, .3f, false, true, {BT_NONE, BT_STRIP, BT_STUTTER, BT_CUT}, false, 32, 16},
    {"WAREHOUSE", F_BERLIN, 134, 51, 2, H_STATIC, K_FOUR, B_ROLLING, S_ONE, L_LOOP, A_DRONE, .9f, .5f, true, false, {BT_KICK, BT_HATS, BT_STRIP, BT_CUT}, true, 16, 14},
    {"ACID TECHNO", F_ACID, 136, 51, 0, H_STATIC, K_FOUR, B_ACID, S_NONE, L_NONE, A_DRONE, .8f, .4f, true, false, {BT_KICK, BT_SNARE, BT_CUT, BT_HATS}, true, 16, 16},
    {"ACID HOUSE", F_ACID, 124, 56, 0, H_MODAL, K_FOUR, B_ACID, S_CHORD, L_NONE, A_PAD, .6f, .4f, true, true, {BT_SNARE, BT_HATS, BT_STRIP, BT_NONE}, false, 16, 16},
    {"HARD TECHNO", F_HARD, 148, 50, 2, H_STATIC, K_FOUR, B_GALLOP, S_ONE, L_LOOP, A_DRONE, 1.f, .6f, true, false, {BT_KICK, BT_STUTTER, BT_SNARE, BT_CUT}, true, 16, 8},
    {"INDUSTRIAL", F_HARD, 138, 50, 2, H_STATIC, K_FOUR, B_ROLLING, S_ONE, L_NONE, A_HIGH, .7f, .8f, false, false, {BT_STUTTER, BT_KICK, BT_POLY, BT_CUT}, true, 16, 16},
    {"SCHRANZ", F_HARD, 152, 50, 0, H_STATIC, K_FOUR, B_GALLOP, S_NONE, L_LOOP, A_DRONE, 1.f, .9f, true, false, {BT_KICK, BT_HATS, BT_CUT, BT_STUTTER}, true, 16, 6},
    {"MELODIC TECHNO", F_MELODIC, 124, 51, 0, H_PROG, K_FOUR, B_ROLLING, S_CHORD, L_ARP, A_PAD, .6f, .4f, true, false, {BT_ARP, BT_SNARE, BT_STRIP, BT_FAKE}, true, 16, 16},
    {"PROGRESSIVE HOUSE", F_MELODIC, 122, 53, 0, H_PROG, K_FOUR, B_OFFBEAT, S_PLUCK, L_MOTIF, A_PAD, .6f, .5f, true, false, {BT_SNARE, BT_ARP, BT_STRIP, BT_HATS}, true, 16, 32},
    {"PEAK TIME", F_BERLIN, 134, 51, 0, H_MODAL, K_FOUR, B_ROLLING, S_CHORD, L_LOOP, A_DRONE, .9f, .6f, true, false, {BT_KICK, BT_SNARE, BT_HATS, BT_STUTTER}, true, 16, 16},
    {"TECH HOUSE", F_HOUSE, 126, 56, 0, H_MODAL, K_FOUR, B_FUNK, S_ONE, L_NONE, A_HIGH, .7f, .8f, true, false, {BT_STRIP, BT_HATS, BT_CUT, BT_NONE}, true, 16, 16},
    {"DEEP HOUSE", F_HOUSE, 122, 57, 1, H_PROG, K_FOUR, B_FUNK, S_CHORD, L_NONE, A_PAD, .5f, .5f, true, true, {BT_NONE, BT_STRIP, BT_HATS, BT_SNARE}, false, 16, 16},
    {"CHICAGO HOUSE", F_HOUSE, 124, 58, 0, H_MODAL, K_FOUR, B_FUNK, S_CHORD, L_NONE, A_PAD, .7f, .4f, true, true, {BT_SNARE, BT_HATS, BT_NONE, BT_CUT}, true, 16, 16},
    {"TRANCE", F_TRANCE, 138, 50, 0, H_PROG, K_FOUR, B_ROLLING, S_PLUCK, L_MOTIF, A_PAD, .7f, .3f, true, false, {BT_SNARE, BT_ARP, BT_FAKE, BT_HATS}, true, 16, 32},
    {"PROGRESSIVE TRANCE", F_TRANCE, 132, 50, 0, H_PROG, K_FOUR, B_ROLLING, S_NONE, L_ARP, A_PAD, .6f, .4f, true, false, {BT_ARP, BT_SNARE, BT_STRIP, BT_NONE}, true, 16, 16},
    {"PSYTRANCE", F_TRANCE, 142, 50, 2, H_STATIC, K_FOUR, B_GALLOP, S_NONE, L_ACID, A_HIGH, .8f, .5f, true, false, {BT_STUTTER, BT_SNARE, BT_KICK, BT_CUT}, true, 16, 16},
    {"ELECTRO", F_ELECTRO, 128, 52, 0, H_MODAL, K_BROKEN, B_FUNK, S_ONE, L_LOOP, A_HIGH, .6f, .5f, false, false, {BT_STUTTER, BT_CUT, BT_SNARE, BT_POLY}, true, 16, 16},
    {"BREAKS", F_ELECTRO, 132, 54, 0, H_MODAL, K_BREAKS, B_FUNK, S_CHORD, L_NONE, A_PAD, .7f, .4f, false, true, {BT_SNARE, BT_CUT, BT_STRIP, BT_HATS}, true, 16, 16},
};

const std::vector<Style>& styles() { return STYLES; }
int numStyles() { return (int) STYLES.size(); }
std::vector<int> stylesOf(int family) {
	std::vector<int> v;
	for (int i = 0; i < numStyles(); i++)
		if (STYLES[i].family == family)
			v.push_back(i);
	return v;
}
static const Style& st(int i) { return STYLES[std::max(0, std::min(numStyles() - 1, i))]; }

static const char* const TYPES_DRUMS[] = {"FOUR ON FLOOR", "ROLLING 4/4", "BROKEN", "BREAKBEAT"};
static const char* const TYPES_BASS[] = {"OFFBEAT", "ROLLING", "ACID 303", "FUNK", "LONG", "GALLOP"};
static const char* const TYPES_STAB[] = {"OFF", "DUB CHORD", "CHORD", "ONE NOTE", "PLUCK"};
static const char* const TYPES_LEAD[] = {"OFF", "HYPNO LOOP", "ARP", "MOTIF", "ACID"};
static const char* const TYPES_ATMO[] = {"DRONE", "PAD", "HIGH DRONE"};
int typeCount(int t) {
	static const int n[NUM_TRACKS] = {4, 6, 5, 5, 3};
	return t >= 0 && t < NUM_TRACKS ? n[t] : 0;
}
const char* typeName(int t, int i) {
	if (i < 0 || i >= typeCount(t))
		return "";
	switch (t) {
	case T_DRUMS: return TYPES_DRUMS[i];
	case T_BASS: return TYPES_BASS[i];
	case T_STAB: return TYPES_STAB[i];
	case T_LEAD: return TYPES_LEAD[i];
	default: return TYPES_ATMO[i];
	}
}
int styleType(int style, int t) {
	const Style& s = st(style);
	switch (t) {
	case T_DRUMS: return s.kick;
	case T_BASS: return s.bass;
	case T_STAB: return s.stab;
	case T_LEAD: return s.lead;
	default: return s.atmo;
	}
}
static int typeOf(const GenSettings& g, int t) { return g.type[t] >= 0 ? g.type[t] : styleType(g.style, t); }

// ------------------------------------------------------------------ harmony
static int scaleNote(int mode, int key, int degree, int baseOct) {
	const Scale& sc = SCALES[mode];
	int oct = degree >= 0 ? degree / sc.n : -((-degree + sc.n - 1) / sc.n);
	int d = degree - oct * sc.n;
	return baseOct * 12 + key + sc.iv[d] + 12 * oct;
}
bool inScale(const Pattern& p, int midi) {
	const Scale& sc = SCALES[p.mode % 5];
	int pc = ((midi - p.key) % 12 + 12) % 12;
	for (int i = 0; i < sc.n; i++)
		if (sc.iv[i] == pc)
			return true;
	return false;
}

static const int PROGS[][4] = {{0, 5, 2, 6}, {0, 3, 5, 4}, {0, 6, 5, 6}, {0, 4, 5, 3}, {5, 6, 0, 0}, {0, 6, 5, 4}};
static const int MODALS[][4] = {{0, 0, 6, 6}, {0, 0, 3, 3}, {0, 0, 5, 5}, {0, 3, 0, 3}};

void makeChords(Pattern& p, int style, int key, uint32_t seed) {
	const Style& s = st(style);
	Rng r(hash(seed, 77));
	p.style = (uint8_t) style;
	p.key = (uint8_t) (key % 12);
	p.mode = (uint8_t) s.mode;
	const int* deg;
	static const int none[4] = {0, 0, 0, 0};
	if (s.harmony == H_PROG)
		deg = PROGS[r.range(6)];
	else if (s.harmony == H_MODAL)
		deg = MODALS[r.range(4)];
	else
		deg = none;
	for (int b = 0; b < 4; b++) {
		int d = deg[b];
		{
			// no diminished chords: the degree next to it instead
			const Scale& sc = SCALES[p.mode % 5];
			int third = (sc.iv[(d + 2) % 7] - sc.iv[d % 7] + 12) % 12;
			int fifth = (sc.iv[(d + 4) % 7] - sc.iv[d % 7] + 12) % 12;
			if (third == 3 && fifth == 6)
				d = (d + 6) % 7;
		}
		p.chordDeg[b] = (uint8_t) d;
		int n = s.rich ? (r.chance(0.5f) ? 4 : 4) : 3;
		int notes[4];
		for (int i = 0; i < n; i++) {
			int dd = d + 2 * i;
			if (s.rich && i == 3 && r.chance(0.5f))
				dd = d + 8; // the 9th instead of the 7th
			notes[i] = scaleNote(p.mode, p.key, dd, 5);
		}
		// close voicing around middle C
		for (int i = 0; i < n; i++) {
			while (notes[i] > 66)
				notes[i] -= 12;
			while (notes[i] < 54)
				notes[i] += 12;
		}
		std::sort(notes, notes + n);
		for (int i = 0; i < VOICES; i++)
			p.chord[b][i] = (int8_t) (i < n ? notes[i] : 0);
		p.chordN[b] = (uint8_t) n;
	}
}

std::string chordName(const Pattern& p, int bar) {
	bar = std::max(0, std::min(3, bar));
	const Scale& sc = SCALES[p.mode % 5];
	int d = p.chordDeg[bar];
	int root = (p.key + sc.iv[d % 7]) % 12;
	int third = (sc.iv[(d + 2) % 7] - sc.iv[d % 7] + 12) % 12;
	int fifth = (sc.iv[(d + 4) % 7] - sc.iv[d % 7] + 12) % 12;
	std::string s = KEY_NAMES[root];
	if (third == 3 && fifth == 6)
		s += "dim";
	else if (third == 3)
		s += "m";
	if (p.chordN[bar] >= 4)
		s += "7";
	return s;
}

static int chordRootPc(const Pattern& p, int bar) {
	const Scale& sc = SCALES[p.mode % 5];
	return (p.key + sc.iv[p.chordDeg[bar & 3] % 7]) % 12;
}
static int bassPitch(const Pattern& p, int bar, int interval = 0) {
	int pc = chordRootPc(p, bar);
	int n = 36 + pc;
	if (n > 44)
		n -= 12;
	return n + interval;
}
/** A scale tone near `center` for the bar's chord: degree offset from the chord root. */
static int chordTone(const Pattern& p, int bar, int idx, int center) {
	int n = p.chordN[bar & 3];
	if (n <= 0)
		return center;
	int i = ((idx % n) + n) % n;
	int oct = idx >= 0 ? idx / n : -((-idx + n - 1) / n);
	int v = p.chord[bar & 3][i] + 12 * oct;
	while (v < center - 7)
		v += 12;
	while (v > center + 14)
		v -= 12;
	return v;
}
static int scaleStep(const Pattern& p, int from, int steps) {
	int v = from;
	int dir = steps > 0 ? 1 : -1;
	for (int k = 0; k != steps; k += dir) {
		v += dir;
		while (!inScale(p, v))
			v += dir;
	}
	return v;
}

// ------------------------------------------------------------------ drums
static void euclid(bool* out, int k, int n, int rot) {
	for (int i = 0; i < n; i++)
		out[(i + rot) % n] = ((i * k) % n) < k;
}

static void genDrums(Pattern& p, const GenSettings& g, Rng& r) {
	const Style& s = st(g.style);
	int type = typeOf(g, T_DRUMS);
	float dens = g.dens[T_DRUMS], groove = g.flavour[T_DRUMS], e = g.energy;
	for (int v = 0; v < DRUMS; v++) {
		for (int i = 0; i < STEPS; i++)
			p.drum[v][i] = Hit();
		p.vlen[v] = 0;
	}
	p.len[T_DRUMS] = 64;
	auto put = [&](int v, int i, int vel, int micro = 0) {
		if (i < 0 || i >= STEPS)
			return;
		p.drum[v][i].vel = (uint8_t) std::max(1, std::min(127, vel));
		p.drum[v][i].micro = (int8_t) micro;
	};
	// kick
	static const int broken[4][4] = {{0, 6, 10, -1}, {0, 3, 10, 14}, {0, 6, 11, -1}, {0, 10, 13, -1}};
	int bk1 = r.range(4), bk2 = r.range(4);
	for (int b = 0; b < 4; b++) {
		int o = b * 16;
		if (type == K_FOUR || type == K_FOUR_GHOST) {
			for (int q = 0; q < 4; q++)
				put(D_KICK, o + q * 4, 118 + r.range(8));
			if (type == K_FOUR_GHOST && (b & 1) && r.chance(0.4f + 0.4f * dens))
				put(D_KICK, o + (r.chance(0.5f) ? 14 : 3), 55 + r.range(15));
		}
		else if (type == K_BROKEN) {
			const int* t = broken[(b & 1) ? bk2 : bk1];
			for (int k = 0; k < 4; k++)
				if (t[k] >= 0)
					put(D_KICK, o + t[k], k ? 105 : 122);
		}
		else { // breaks
			put(D_KICK, o, 122);
			put(D_KICK, o + 10, 110);
			if (b & 1)
				put(D_KICK, o + 7, 96);
		}
	}
	// backbeat
	bool backSnare = !s.clap && (type == K_BROKEN || type == K_BREAKS || s.family == F_ELECTRO);
	for (int b = 0; b < 4; b++) {
		int o = b * 16;
		if (s.clap) {
			put(D_CLAP, o + 4, 108 + r.range(10), -3);
			put(D_CLAP, o + 12, 110 + r.range(10), -3);
			if (b == 3 && r.chance(0.3f * dens))
				put(D_CLAP, o + 15, 70);
		}
		else if (backSnare) {
			put(D_SNARE, o + 4, 115);
			put(D_SNARE, o + 12, 118);
			if (r.chance(0.5f * dens))
				put(D_SNARE, o + (r.chance(0.5f) ? 7 : 15), 40 + r.range(15));
		}
		else {
			// no backbeat: a few syncopated snare/rim hits (hypnotic, dub, industrial)
			static const int syn[5] = {3, 7, 11, 14, 15};
			if (r.chance(0.35f + 0.4f * dens))
				put(D_SNARE, o + syn[r.range(5)], 55 + r.range(25));
		}
	}
	// rim: syncopated, more in minimal and detroit
	{
		static const int pos[6] = {3, 6, 10, 13, 14, 7};
		float pr = (s.family == F_BERLIN || s.family == F_DETROIT || s.family == F_HOUSE ? 0.5f : 0.25f) * (0.4f + dens);
		int a = pos[r.range(6)], bb = pos[r.range(6)];
		for (int b = 0; b < 4; b++) {
			if (r.chance(pr))
				put(D_RIM, b * 16 + a, 70 + r.range(30));
			if ((b & 1) && r.chance(pr * 0.7f))
				put(D_RIM, b * 16 + bb, 60 + r.range(25));
		}
	}
	// hats: closed in 16ths or 8ths, open on the offbeats
	for (int i = 0; i < STEPS; i++) {
		int q = i % 4;
		int micro = (int) (groove * (q == 2 ? 10 : 6)) + r.range(3) - 1;
		if (q == 2) {
			bool oh = type != K_BREAKS || (i % 16) == 14;
			if (s.family == F_BERLIN && s.harmony == H_MODAL && (i / 4) % 2 == 0)
				oh = false; // dub: fewer open hats
			if (oh)
				put(D_OH, i, 92 + r.range(14), micro);
			else
				put(D_CH, i, 96, micro);
			continue;
		}
		if (s.fullHat || (i % 2) == 0) {
			float keep = (q == 0 ? 0.85f : 0.55f) + 0.4f * s.hats * dens;
			if (r.chance(keep))
				put(D_CH, i, (q == 0 ? 64 : 48) + r.range(20) + (int) (18 * groove * (i % 2)), micro);
		}
	}
	if (s.family == F_HARD && r.chance(0.5f)) // hard: a 32nd roll on the closed hat now and then
		p.drum[D_CH][14 + 32].ratch = 2;
	// tom: sparse, a bit more in hard styles
	{
		float pt = (s.family == F_HARD ? 0.5f : 0.2f) * (0.3f + dens);
		for (int b = 1; b < 4; b += 2)
			if (r.chance(pt))
				put(D_TOM, b * 16 + (r.chance(0.5f) ? 11 : 14), 80 + r.range(25));
	}
	// perc: a euclidean figure over its own length (polymeter)
	{
		static const int lens[6] = {16, 12, 10, 6, 24, 14};
		int L = lens[r.range(6)];
		if (s.family == F_HOUSE || s.family == F_TRANCE)
			L = r.chance(0.6f) ? 16 : 12;
		int k = std::max(1, (int) std::round(L * (0.15f + 0.3f * s.perc * (0.4f + dens))));
		bool hit[24] = {};
		euclid(hit, k, L, 1 + r.range(3));
		for (int i = 0; i < L; i++)
			if (hit[i] && (i % 4) != 0)
				put(D_PERC, i, 70 + r.range(40), r.range(5) - 2);
		p.vlen[D_PERC] = (uint8_t) L;
	}
	(void) e;
}

static float scoreDrums(const Pattern& p, const GenSettings& g) {
	const Style& s = st(g.style);
	int hits = 0, kickClash = 0;
	for (int v = 0; v < DRUMS; v++)
		for (int i = 0; i < 64; i++)
			if (p.drum[v][i].vel) {
				hits++;
				if (v == D_PERC && p.drum[D_KICK][i].vel)
					kickClash++;
			}
	float target = 64 * (0.6f + 0.5f * s.hats * (0.5f + g.dens[T_DRUMS]) + 0.2f * g.energy);
	float sc = -std::fabs(hits - target) / 8.f - kickClash * 0.5f;
	// bars 1 and 3 alike (repetition), bar 4 a little different
	int same13 = 0, diff4 = 0;
	for (int v = 0; v < DRUMS; v++)
		for (int i = 0; i < 16; i++) {
			same13 += (p.drum[v][i].vel > 0) == (p.drum[v][32 + i].vel > 0);
			diff4 += (p.drum[v][i].vel > 0) != (p.drum[v][48 + i].vel > 0);
		}
	sc += same13 / 32.f;
	sc -= std::fabs(diff4 - 4) / 4.f;
	return sc;
}

// ------------------------------------------------------------------ melodic tracks
static void setNote(Note& n, int pitch, int len, int vel, bool acc = false, bool slide = false) {
	n = Note();
	n.on = 1;
	n.pitch[0] = (int8_t) std::max(0, std::min(127, pitch));
	n.n = 1;
	n.len = (uint8_t) std::max(1, std::min(64, len));
	n.vel = (uint8_t) std::max(1, std::min(127, vel));
	n.acc = acc;
	n.slide = slide;
}
static void setChord(Note& n, const Pattern& p, int bar, int len, int vel, int center) {
	n = Note();
	n.on = 1;
	int c = p.chordN[bar & 3];
	int shift = 0;
	if (c > 0) {
		int mid = (p.chord[bar & 3][0] + p.chord[bar & 3][c - 1]) / 2;
		while (mid + shift < center - 6)
			shift += 12;
		while (mid + shift > center + 6)
			shift -= 12;
	}
	for (int i = 0; i < VOICES; i++)
		n.pitch[i] = (int8_t) (i < c ? p.chord[bar & 3][i] + shift : 0);
	n.n = (uint8_t) std::max(1, c);
	n.len = (uint8_t) std::max(1, std::min(64, len));
	n.vel = (uint8_t) vel;
}

static void genBass(Pattern& p, const GenSettings& g, Rng& r) {
	const Style& s = st(g.style);
	Note* m = p.mel[T_BASS - 1];
	for (int i = 0; i < STEPS; i++)
		m[i] = Note();
	int type = typeOf(g, T_BASS);
	float dens = g.dens[T_BASS], fl = g.flavour[T_BASS];
	p.len[T_BASS] = 64;
	const Scale& sc = SCALES[p.mode % 5];
	int fifth = 7, oct = 12, b7 = sc.iv[6], m3 = sc.iv[2];
	if (type == B_ACID) {
		// a 303 line: one bar, repeated, bar 4 varied
		int line[16];
		bool on[16], acc[16], sl[16];
		int pool[6] = {0, 0, oct, m3, fifth, b7};
		if (p.mode == 2)
			pool[3] = 1; // phrygian: the flat second
		for (int i = 0; i < 16; i++) {
			on[i] = r.chance(0.5f + 0.35f * dens);
			line[i] = pool[r.range(6)];
			acc[i] = r.chance(0.2f + 0.2f * fl);
			sl[i] = false;
		}
		on[0] = true;
		line[0] = 0;
		for (int i = 0; i < 15; i++)
			if (on[i] && on[i + 1] && r.chance(0.12f + 0.35f * fl))
				sl[i] = true;
		int L = r.chance(0.3f) ? (r.chance(0.5f) ? 12 : 14) : 16; // sometimes an odd length (acid loops shift)
		for (int b = 0; b < 4; b++)
			for (int i = 0; i < 16; i++) {
				int idx = b * 16 + i;
				bool o = on[i];
				int iv = line[i];
				if (b == 3 && i >= 8 && r.chance(0.3f)) {
					o = r.chance(0.7f);
					iv = pool[r.range(6)];
				}
				if (o)
					setNote(m[idx], bassPitch(p, b, iv), 1, acc[i] ? 120 : 92, acc[i], sl[i]);
			}
		p.len[T_BASS] = (uint8_t) (L == 16 ? 64 : L);
		return;
	}
	for (int b = 0; b < 4; b++) {
		int o = b * 16;
		int root = bassPitch(p, b);
		switch (type) {
		case B_OFFBEAT:
			for (int q = 0; q < 4; q++) {
				int i = o + q * 4 + 2;
				int pitch = root;
				if (q == 3 && b == 3 && r.chance(0.3f + 0.4f * fl))
					pitch = root + (r.chance(0.5f) ? oct : fifth);
				setNote(m[i], pitch, g.energy > 0.7f ? 1 : 2, 100);
				if (dens > 0.6f && r.chance(dens - 0.5f))
					setNote(m[i + 1], root, 1, 80);
			}
			break;
		case B_ROLLING:
		case B_GALLOP:
			for (int i = 0; i < 16; i++) {
				int q = i % 4;
				if (q == 0)
					continue;
				bool keep = type == B_GALLOP ? true : (q == 2 || r.chance(0.35f + 0.6f * dens));
				if (!keep)
					continue;
				int pitch = root;
				if (q == 3 && r.chance(0.15f * fl + (b == 3 ? 0.15f : 0.f)))
					pitch = root + (r.chance(0.6f) ? oct : fifth);
				setNote(m[o + i], pitch, 1, q == 1 ? 105 : (q == 2 ? 92 : 98));
			}
			break;
		case B_FUNK: {
			static const int tpl[4][6] = {{3, 6, 10, 11, 14, -1}, {2, 7, 10, 13, -1, -1}, {3, 7, 10, 14, 15, -1}, {2, 6, 7, 11, 14, -1}};
			const int* t = tpl[(b == 3) ? r.range(4) : ((int) (p.seed % 4))];
			for (int k = 0; k < 6 && t[k] >= 0; k++) {
				if (k > 1 && !r.chance(0.5f + 0.5f * dens))
					continue;
				int ivs[5] = {0, 0, oct, fifth, b7};
				int iv = k == 0 ? 0 : ivs[r.range(5)];
				setNote(m[o + t[k]], root + iv, r.chance(0.5f) ? 1 : 2, 90 + r.range(30), r.chance(0.2f * fl));
			}
			break;
		}
		case B_LONG:
			setNote(m[o + (r.chance(0.5f) ? 2 : 3)], root, 6, 100);
			setNote(m[o + 10], root + (r.chance(0.25f * fl) ? fifth : 0), 4, 90);
			break;
		}
	}
}

static float scoreBass(const Pattern& p, const GenSettings& g) {
	int notes = 0, clash = 0;
	const Note* m = p.mel[T_BASS - 1];
	int type = typeOf(g, T_BASS);
	for (int i = 0; i < 64; i++) {
		if (!m[i].on)
			continue;
		notes++;
		if (p.drum[D_KICK][i].vel > 100)
			clash++;
	}
	float sc = -clash * (type == B_ACID ? 0.4f : 2.f);
	// repetition: bar 1 and bar 2 mostly the same
	int same = 0;
	for (int i = 0; i < 16; i++)
		same += (m[i].on > 0) == (m[16 + i].on > 0);
	sc += same / 8.f;
	float target = 64 * (0.15f + 0.45f * g.dens[T_BASS]);
	if (type == B_LONG)
		target = 8;
	sc -= std::fabs(notes - target) / 10.f;
	return sc;
}

static void genStab(Pattern& p, const GenSettings& g, Rng& r) {
	const Style& s = st(g.style);
	Note* m = p.mel[T_STAB - 1];
	for (int i = 0; i < STEPS; i++)
		m[i] = Note();
	int type = typeOf(g, T_STAB);
	float dens = g.dens[T_STAB], fl = g.flavour[T_STAB];
	p.len[T_STAB] = 64;
	if (type == S_NONE)
		return;
	if (type == S_ONE) {
		int L = s.stabLen > 16 ? s.stabLen : 16;
		if (r.chance(0.4f))
			L = r.chance(0.5f) ? 24 : 20;
		int hits = 1 + (dens > 0.5f) + (dens > 0.8f);
		int base = chordTone(p, 0, r.chance(0.6f) ? 0 : 2, 64);
		for (int k = 0; k < hits; k++) {
			int i = (r.range(L / 2) * 2 + 2 + (r.chance(0.5f) ? 1 : 0)) % L;
			if (i % 4 == 0)
				i = (i + 3) % L;
			setNote(m[i], base + (k == 2 ? 7 : 0), 1, 95 + r.range(20));
			if (r.chance(0.5f)) { // a dyad
				m[i].pitch[1] = (int8_t) (base + 7);
				m[i].n = 2;
			}
		}
		p.len[T_STAB] = (uint8_t) std::min(64, L);
		return;
	}
	for (int b = 0; b < 4; b++) {
		int o = b * 16;
		if (type == S_DUB) {
			static const int pos[5] = {3, 6, 10, 13, 14};
			int hits = 1 + (r.chance(dens) ? 1 : 0) + ((b & 1) && r.chance(dens * 0.5f) ? 1 : 0);
			for (int k = 0; k < hits; k++) {
				int i = o + pos[r.range(5)];
				setChord(m[i], p, b, 1, 100, 60);
				// echoes written as notes (dub throws), DUB knob
				if (fl > 0.3f && i + 3 < o + 16 && !m[i + 3].on) {
					setChord(m[i + 3], p, b, 1, (int) (70 * fl), 60);
				}
			}
		}
		else if (type == S_CHORD) {
			static const int tpl[4][5] = {{0, 3, 6, 10, 12}, {2, 6, 10, 14, -1}, {0, 3, 7, 10, -1}, {3, 6, 11, 14, -1}};
			const int* t = tpl[(b == 3 && r.chance(0.5f)) ? r.range(4) : (int) (p.seed % 4)];
			for (int k = 0; k < 5 && t[k] >= 0; k++) {
				if (k > 1 && !r.chance(0.4f + 0.6f * dens))
					continue;
				setChord(m[o + t[k]], p, b, r.chance(0.5f) ? 1 : 2, 85 + r.range(30), 62);
			}
		}
		else if (type == S_PLUCK) {
			for (int q = 0; q < 4; q++)
				setChord(m[o + q * 4 + 2], p, b, 1, 100, 64);
			if (dens > 0.5f)
				for (int k = 0; k < 2; k++)
					if (r.chance(dens - 0.3f))
						setChord(m[o + (r.chance(0.5f) ? 7 : 11) + (k ? 4 : 0) % 16], p, b, 1, 80, 64);
		}
	}
}

static void genLead(Pattern& p, const GenSettings& g, Rng& r) {
	const Style& s = st(g.style);
	Note* m = p.mel[T_LEAD - 1];
	for (int i = 0; i < STEPS; i++)
		m[i] = Note();
	int type = typeOf(g, T_LEAD);
	float dens = g.dens[T_LEAD], fl = g.flavour[T_LEAD];
	p.len[T_LEAD] = 64;
	if (type == L_NONE)
		return;
	if (type == L_LOOP) {
		// a short loop of a few close notes, its own length against the bar
		int L = s.leadLen;
		if (fl < 0.3f)
			L = std::max(6, L - 4);
		else if (fl > 0.7f)
			L = std::min(16, L + 4);
		int n = std::max(2, (int) std::round(L * (0.2f + 0.3f * dens)));
		int pitch = chordTone(p, 0, 0, 67);
		for (int k = 0; k < n; k++) {
			int i = r.range(L);
			if (m[i].on)
				continue;
			setNote(m[i], pitch, 1, 90 + r.range(30), r.chance(0.3f));
			pitch = scaleStep(p, pitch, r.range(5) - 2);
			pitch = std::max(55, std::min(79, pitch));
		}
		p.len[T_LEAD] = (uint8_t) L;
		return;
	}
	if (type == L_ARP) {
		static const int shapes[4][6] = {{0, 1, 2, 3, 2, 1}, {0, 2, 1, 3, 4, 2}, {3, 2, 1, 0, 1, 2}, {0, 1, 2, 0, 1, 2}};
		const int* sh = shapes[r.range(4)];
		int every = dens < 0.35f ? 2 : 1;
		int center = 72;
		for (int b = 0; b < 4; b++)
			for (int i = 0; i < 16; i += every) {
				int idx = b * 16 + i;
				int tone = sh[(idx / every) % 6];
				setNote(m[idx], chordTone(p, b, tone, center), 1, (i % 4 == 0) ? 105 : 85);
			}
		return;
	}
	if (type == L_ACID) {
		int pool[5] = {0, 12, 3, 7, 10};
		int base = 60 + p.key;
		for (int i = 0; i < 16; i++) {
			if (!r.chance(0.55f + 0.35f * dens))
				continue;
			int pitch = base + pool[r.range(5)];
			if (!inScale(p, pitch))
				pitch = scaleStep(p, pitch, 1);
			bool a = r.chance(0.3f), sl = r.chance(0.2f + 0.3f * fl);
			for (int b = 0; b < 4; b++)
				setNote(m[b * 16 + i], pitch, 1, a ? 118 : 92, a, sl);
		}
		return;
	}
	// MOTIF: question (bars 1-2) and answer (bars 3-4) on the same rhythm
	static const int rh[3][6][2] = {{{0, 3}, {3, 1}, {4, 4}, {10, 2}, {12, 4}, {-1, 0}},
	                                {{0, 2}, {2, 2}, {6, 4}, {12, 2}, {14, 2}, {-1, 0}},
	                                {{0, 4}, {6, 2}, {8, 4}, {14, 2}, {-1, 0}, {-1, 0}}};
	int a = r.range(3), bR = r.range(3);
	int center = 72;
	int pitch = chordTone(p, 0, 2, center);
	for (int half = 0; half < 2; half++)
		for (int b = 0; b < 2; b++) {
			int bar = half * 2 + b;
			const int(*t)[2] = rh[b == 0 ? a : bR];
			for (int k = 0; k < 6 && t[k][0] >= 0; k++) {
				if (k > 2 && dens < 0.3f)
					continue;
				int i = bar * 16 + t[k][0];
				bool strong = t[k][0] % 8 == 0;
				if (strong)
					pitch = chordTone(p, bar, r.range(3), center);
				else
					pitch = scaleStep(p, pitch, r.range(5) - 2);
				pitch = std::max(64, std::min(86, pitch));
				if (half == 1 && b == 1 && k == 0)
					pitch = chordTone(p, bar, r.chance(0.5f) ? 0 : 2, center); // the answer ends on the chord
				setNote(m[i], pitch, t[k][1], strong ? 105 : 90);
			}
		}
}

static float scoreLead(const Pattern& p, const GenSettings& g) {
	const Note* m = p.mel[T_LEAD - 1];
	float sc = 0;
	int prev = -1, n = 0;
	for (int i = 0; i < 64; i++) {
		if (!m[i].on)
			continue;
		n++;
		int pit = m[i].pitch[0];
		if (prev >= 0) {
			int leap = std::abs(pit - prev);
			if (leap > 7)
				sc -= (leap - 7) * 0.3f;
		}
		// chord tone on a strong beat
		if (i % 4 == 0) {
			int bar = (i / 16) & 3;
			bool ct = false;
			for (int k = 0; k < p.chordN[bar]; k++)
				if ((pit - p.chord[bar][k]) % 12 == 0)
					ct = true;
			sc += ct ? 0.5f : -0.3f;
		}
		// stab and lead apart
		const Note& sn = p.mel[T_STAB - 1][i];
		if (sn.on && std::abs(sn.pitch[0] - pit) < 3)
			sc -= 0.4f;
		prev = pit;
	}
	(void) g;
	return sc + (n > 0 ? 0 : -5);
}

static void genAtmo(Pattern& p, const GenSettings& g, Rng& r) {
	Note* m = p.mel[T_ATMO - 1];
	for (int i = 0; i < STEPS; i++)
		m[i] = Note();
	int type = typeOf(g, T_ATMO);
	p.len[T_ATMO] = 64;
	// runs of the same chord become one long note
	int b = 0;
	while (b < 4) {
		int e = b + 1;
		while (e < 4 && p.chordDeg[e] == p.chordDeg[b])
			e++;
		int L = (e - b) * 16;
		Note& n = m[b * 16];
		if (type == A_PAD)
			setChord(n, p, b, L, 80, 60);
		else if (type == A_HIGH) {
			setNote(n, chordTone(p, b, 0, 76), L, 70);
			n.pitch[1] = (int8_t) (n.pitch[0] + 7);
			n.n = 2;
		}
		else {
			setNote(n, chordTone(p, b, 0, 48), L, 80);
			n.pitch[1] = (int8_t) (n.pitch[0] + 7);
			n.n = 2;
		}
		b = e;
	}
	(void) r;
}

// ------------------------------------------------------------------ generate / mutate
template <typename G, typename S>
static void bestOf(Pattern& p, const GenSettings& g, uint32_t seed, int salt, int tries, G gen, S score) {
	Pattern best = p;
	float bestSc = -1e9f;
	for (int k = 0; k < tries; k++) {
		Pattern c = p;
		Rng r(hash(seed, salt, k));
		gen(c, g, r);
		float sc = score(c, g) + Rng(hash(seed, salt + 99, k)).uni() * 0.2f;
		if (sc > bestSc) {
			bestSc = sc;
			best = c;
		}
	}
	p = best;
}
static float scoreNone(const Pattern&, const GenSettings&) { return 0.f; }

void generate(Pattern& p, const GenSettings& g, uint32_t seed, int track) {
	if (track < 0) {
		p = Pattern();
		p.seed = seed;
		makeChords(p, g.style, g.key, seed);
	}
	else if (p.style != g.style || p.key != g.key)
		makeChords(p, g.style, g.key, p.seed);
	if (track < 0 || track == T_DRUMS)
		bestOf(p, g, seed, 1, 8, genDrums, scoreDrums);
	if (track < 0 || track == T_BASS)
		bestOf(p, g, seed, 2, 10, genBass, scoreBass);
	if (track < 0 || track == T_STAB)
		bestOf(p, g, seed, 3, 4, genStab, scoreNone);
	if (track < 0 || track == T_LEAD)
		bestOf(p, g, seed, 4, 10, genLead, scoreLead);
	if (track < 0 || track == T_ATMO)
		bestOf(p, g, seed, 5, 1, genAtmo, scoreNone);
	// the per-track lengths the user set win over the generated ones (0: keep)
	for (int t = 0; t < NUM_TRACKS; t++)
		if ((track < 0 || track == t) && g.len[t] > 0)
			p.len[t] = (uint8_t) g.len[t];
}

void mutate(Pattern& p, const GenSettings& g, uint32_t seed, int track, float amount) {
	Rng r(hash(seed, 555, track));
	int changes = 1 + (int) (amount * 3);
	for (int k = 0; k < changes; k++) {
		if (track == T_DRUMS) {
			static const int vs[4] = {D_CH, D_PERC, D_RIM, D_SNARE};
			int v = vs[r.range(4)];
			int i = r.range(64);
			if (i % 4 == 0 && v != D_CH)
				i++;
			Hit& h = p.drum[v][i];
			h.vel = h.vel ? 0 : (uint8_t) (55 + r.range(50));
		}
		else if (track >= T_BASS && track <= T_LEAD) {
			Note* m = p.mel[track - 1];
			int L = p.trackLen(track);
			int i = r.range(std::max(1, L));
			if (m[i].on && r.chance(0.6f)) {
				if (track == T_STAB)
					m[i].on = 0;
				else
					m[i].pitch[0] = (int8_t) scaleStep(p, m[i].pitch[0], r.chance(0.5f) ? 1 : -1);
			}
			else if (!m[i].on && (i % 4) != 0) {
				int ref = -1;
				for (int j = 0; j < L && ref < 0; j++)
					if (m[j].on)
						ref = j;
				if (ref >= 0) {
					m[i] = m[ref];
					m[i].len = 1;
					m[i].slide = 0;
				}
			}
		}
	}
	(void) g;
}

// ------------------------------------------------------------------ the song
int songBars(const std::vector<Section>& s) {
	int n = 0;
	for (const Section& x : s)
		n += x.bars;
	return n;
}

int pickBuild(int style, uint32_t seed, int avoid, int grow) {
	const Style& s = st(style);
	static const float w[4] = {4, 3, 2, 1.5f};
	Rng r(hash(seed, 31, grow));
	for (int tries = 0; tries < 8; tries++) {
		float tot = 0;
		for (int i = 0; i < 4; i++)
			tot += w[i];
		float x = r.uni() * tot;
		int k = 0;
		while (k < 3 && x > w[k]) {
			x -= w[k];
			k++;
		}
		int b = s.builds[k];
		if (b == avoid)
			continue;
		if (grow > 0 && (b == BT_NONE || b == BT_STRIP || b == BT_CUT) && tries < 6)
			continue; // the second build is a stronger one
		return b;
	}
	return s.builds[0] == avoid ? s.builds[1] : s.builds[0];
}

std::vector<Section> planSong(int style, float minutes, float bpm, uint32_t seed, int forcedBuild, bool grow, bool djIntro) {
	const Style& s = st(style);
	Rng r(hash(seed, 4242));
	struct T {
		int part, bars, flex; // flex: may grow or shrink
	};
	std::vector<T> tpl;
	bool hypnotic = s.family == F_BERLIN && style != 13; // not PEAK TIME
	if (hypnotic)
		tpl = {{P_INTRO, 32, 0}, {P_GROOVE, 64, 1}, {P_BREAK, 16, 0}, {P_BUILD, 8, 0}, {P_DROP, 64, 1}, {P_OUTRO, 32, 0}};
	else if (s.family == F_MELODIC || s.family == F_TRANCE)
		tpl = {{P_INTRO, 32, 0}, {P_GROOVE, 16, 1}, {P_BUILD, 8, 0}, {P_DROP, 32, 1},
		       {P_BREAK, 32, 0}, {P_BUILD, 16, 0}, {P_DROP, 32, 1}, {P_OUTRO, 32, 0}};
	else if (s.family == F_HARD)
		tpl = {{P_INTRO, 16, 0}, {P_GROOVE, 32, 1}, {P_BUILD, 8, 0}, {P_DROP, 32, 1},
		       {P_BREAK, 8, 0}, {P_BUILD, 8, 0}, {P_DROP, 32, 1}, {P_OUTRO, 16, 0}};
	else
		tpl = {{P_INTRO, 32, 0}, {P_GROOVE, 32, 1}, {P_BUILD, 8, 0}, {P_DROP, 32, 1}, {P_BREAK, 16, 0},
		       {P_BUILD, 8, 0}, {P_DROP, 32, 1}, {P_GROOVE, 16, 1}, {P_OUTRO, 32, 0}};
	// a fake build now and then (styles that like it): build › break › build › drop
	bool fake = false;
	for (int i = 0; i < 4; i++)
		fake = fake || s.builds[i] == BT_FAKE;
	if (fake && forcedBuild == B_AUTO && r.chance(0.5f)) {
		for (size_t i = 0; i < tpl.size(); i++)
			if (tpl[i].part == P_BREAK) {
				tpl.insert(tpl.begin() + i + 1, T{P_BUILD, 8, -1}); // -1: fake
				tpl.insert(tpl.begin() + i + 2, T{P_BREAK, 8, 0});
				break;
			}
	}
	// a short intro unless a DJ intro is wanted (the time goes to the grooves and drops)
	if (!djIntro)
		for (T& t : tpl)
			if (t.part == P_INTRO)
				t.bars = 16;
	int want = std::max(64, (int) std::round(minutes * bpm / 4.f / 8.f) * 8);
	int have = 0;
	for (const T& t : tpl)
		have += t.bars;
	// fit the length: flexible sections in steps of 8 bars
	for (int guard = 0; have != want && guard < 200; guard++) {
		bool changed = false;
		for (T& t : tpl) {
			if (have == want)
				break;
			if (t.flex == 1) {
				if (have < want && t.bars < 128) {
					t.bars += 8;
					have += 8;
					changed = true;
				}
				else if (have > want && t.bars > 16) {
					t.bars -= 8;
					have -= 8;
					changed = true;
				}
			}
		}
		if (!changed) {
			// still too long: shorten intro, outro and breaks down to 8
			for (T& t : tpl)
				if (have > want && (t.part == P_INTRO || t.part == P_OUTRO || t.part == P_BREAK) && t.bars > 8) {
					t.bars -= 8;
					have -= 8;
					changed = true;
				}
			if (!changed)
				break;
		}
	}
	std::vector<Section> out;
	int builds = 0, drops = 0, lastBuild = -1;
	for (const T& t : tpl) {
		Section x;
		x.part = (uint8_t) t.part;
		x.bars = (uint8_t) std::min(255, t.bars);
		if (t.part == P_BUILD) {
			int g = grow ? std::min(builds, 2) : 0;
			if (t.flex == -1)
				x.build = BT_FAKE;
			else {
				x.build = (uint8_t) (forcedBuild != B_AUTO ? forcedBuild : pickBuild(style, hash(seed, builds), lastBuild, g));
				x.grow = (uint8_t) g;
				if (g > 0 && x.bars < 16)
					x.bars = 16; // GROW: the later build is longer
				lastBuild = x.build;
				builds++;
			}
		}
		if (t.part == P_DROP) {
			x.dropIdx = (uint8_t) drops;
			x.grow = (uint8_t) (grow ? std::min(drops, 2) : 0);
			drops++;
		}
		out.push_back(x);
	}
	return out;
}

// ------------------------------------------------------------------ playing
// the elements in the order they come in (intro) or go (outro)
enum Elem { E_KICK, E_CH, E_OH, E_PERC, E_BACK, E_RIM, E_BASS, E_STAB, E_LEAD, E_ATMO };
static int elemOf(int track, int voice) {
	if (track != T_DRUMS)
		return E_BASS + (track - T_BASS);
	switch (voice) {
	case D_KICK: return E_KICK;
	case D_SNARE:
	case D_CLAP: return E_BACK;
	case D_RIM:
	case D_TOM: return E_RIM;
	case D_CH: return E_CH;
	case D_OH: return E_OH;
	default: return E_PERC;
	}
}

bool elementIn(int style, const Ctx& c, int track, int voice) {
	const Style& s = st(style);
	int e = elemOf(track, voice);
	int phrase = c.secBar / 8, phrases = std::max(1, (c.secBars + 7) / 8);
	static const int introOrder[6] = {E_KICK, E_CH, E_OH, E_PERC, E_BACK, E_RIM};
	auto rankIn = [&](const int* order, int n) {
		for (int i = 0; i < n; i++)
			if (order[i] == e)
				return i;
		return 99;
	};
	switch (c.part) {
	case P_INTRO: {
		if (e >= E_BASS) {
			if (c.djEdges)
				return false;
			// without DJ edges: the atmo from the second phrase, the bass in the last one
			if (e == E_ATMO)
				return phrase >= 1;
			if (e == E_BASS)
				return phrase == phrases - 1 && phrases > 1;
			return false;
		}
		int n = 2 + phrase * (phrases <= 2 ? 3 : 1);
		return rankIn(introOrder, 6) < n;
	}
	case P_OUTRO: {
		if (e >= E_BASS) {
			if (c.djEdges)
				return phrase == 0 && e == E_BASS && phrases > 2; // the bass leaves after the first phrase
			return phrase < phrases - 1 && e != E_LEAD;
		}
		int left = phrases - phrase; // phrases still to come, including this one
		int n = 2 + (left - 1) * (phrases <= 2 ? 3 : 1);
		return rankIn(introOrder, 6) < n;
	}
	case P_GROOVE: {
		if (e < E_BASS)
			return true;
		// one element more every 8 bars (after a drop: all of them)
		static const int melOrder[4] = {E_BASS, E_STAB, E_LEAD, E_ATMO};
		int n = c.dropIdx > 0 ? 4 : 2 + phrase * 2; // bass and stab at once, all of them from the second phrase
		if (s.family == F_BERLIN || s.family == F_HARD)
			n = std::max(n, 2);
		return rankIn(melOrder, 4) < n;
	}
	case P_BUILD: {
		if (e == E_BASS)
			return c.build == BT_NONE || c.build == BT_POLY || c.build == BT_HATS ? (c.secBar < c.secBars - 2) : false;
		if (e == E_KICK)
			return c.build != BT_SNARE || c.grow == 0 ? true : c.secBar < c.secBars - 2;
		return true;
	}
	case P_DROP: {
		// a short bass pause in long drops (bars 15-16), back with a fill
		if (e == E_BASS && c.secBars >= 32 && (c.secBar == 14 || c.secBar == 15))
			return false;
		return true;
	}
	case P_BREAK: {
		if (e == E_KICK || e == E_BASS || e == E_BACK)
			return false;
		if (e >= E_STAB)
			return true;
		// drums in the break: house keeps a pulse, techno keeps the hats, melodic styles go to nothing
		if (s.family == F_HOUSE)
			return e == E_CH || e == E_PERC;
		if (s.family == F_BERLIN || s.family == F_HARD || s.family == F_ACID)
			return e == E_CH || (e == E_PERC && phrase > 0);
		return false;
	}
	}
	return true;
}

static int loopIdx(int64_t step, int len) {
	len = std::max(1, std::min(64, len));
	return (int) (step % len);
}

void resolveStep(const Pattern& p, const Ctx& c, StepOut& out) {
	out = StepOut();
	const Style& s = st(p.style);
	int sib = (int) (c.songStep % 16);   // step in the bar
	Rng r(hash(c.rnd, (uint32_t) c.songStep, 17));
	bool lastBar = c.secBar == c.secBars - 1;
	float prog = (c.secBar * 16 + sib) / std::max(1.f, c.secBars * 16.f);
	bool build = c.part == P_BUILD;
	// ---- drums from the pattern
	for (int v = 0; v < DRUMS; v++) {
		out.elementOn[NUM_TRACKS + v] = elementIn(p.style, c, T_DRUMS, v);
		if (!out.elementOn[NUM_TRACKS + v])
			continue;
		const Hit& h = p.drum[v][loopIdx(c.songStep, p.voiceLen(v))];
		if (!h.vel)
			continue;
		int vel = h.vel;
		// ENERGY: ghosts and busy hats thin out when it is low
		if (v == D_CH || v == D_PERC || v == D_RIM || v == D_TOM) {
			if (vel < 40 + (int) ((1.f - c.energy) * 40.f))
				continue;
			if (v == D_CH && c.energy < 0.35f && (sib & 1))
				continue;
			if (v == D_PERC && r.uni() > 0.4f + c.energy)
				continue;
		}
		if (h.chance < 100 && r.range(100) >= h.chance)
			continue;
		out.drum[v].vel = (uint8_t) vel;
		out.drum[v].ratch = h.ratch;
		out.drum[v].micro = h.micro;
	}
	// ---- melodic tracks
	for (int t = T_BASS; t < NUM_TRACKS; t++) {
		out.elementOn[t] = elementIn(p.style, c, t, 0);
		if (!out.elementOn[t])
			continue;
		const Note& n = p.mel[t - 1][loopIdx(c.songStep, p.trackLen(t))];
		if (!n.on)
			continue;
		if (n.chance < 100 && r.range(100) >= n.chance)
			continue;
		out.mel[t - 1].on = true;
		out.mel[t - 1].note = n;
		if (t == T_STAB || t == T_LEAD)
			out.mel[t - 1].note.vel = (uint8_t) std::max(1, (int) (n.vel * (0.8f + 0.2f * c.energy)));
	}
	out.elementOn[T_DRUMS] = true;
	auto hit = [&](int v, int vel, int ratch = 1) {
		out.drum[v].vel = (uint8_t) std::max(1, std::min(127, vel));
		out.drum[v].ratch = (uint8_t) ratch;
		out.drum[v].micro = 0;
	};
	// ---- builds
	if (build) {
		out.rise = prog;
		int g = c.grow;
		int bt = c.build;
		float rollFrom = c.secBars > 8 ? 1.f - 8.f / c.secBars : 0.f; // rolls in the last 8 bars
		float rp = rollFrom < 1.f ? std::max(0.f, (prog - rollFrom) / (1.f - rollFrom)) : prog;
		auto roll = [&](int v) {
			if (prog < rollFrom)
				return;
			int every = rp < 0.5f ? 4 : (rp < 0.75f ? 2 : 1);
			if (sib % every == 0) {
				int ratch = (rp > 0.9f && g > 0) ? 2 : 1;
				hit(v, 40 + (int) (80 * rp), ratch);
			}
		};
		switch (bt) {
		case BT_SNARE:
		case BT_FAKE: roll(D_SNARE); break;
		case BT_KICK: {
			int every = prog < 0.5f ? 4 : (prog < 0.8f ? 2 : 1);
			if (sib % every == 0)
				hit(D_KICK, 70 + (int) (50 * prog));
			else
				out.drum[D_KICK].vel = 0;
			break;
		}
		case BT_HATS: {
			out.drum[D_OH].vel = 0;
			int ratch = prog < 0.5f ? 1 : (prog < 0.85f ? 2 : 3);
			hit(D_CH, 50 + (int) (70 * prog), ratch);
			if (g > 0)
				roll(D_SNARE);
			break;
		}
		case BT_ARP: {
			NoteFire& l = out.mel[T_LEAD - 1];
			if (l.on)
				l.note.ratch = (uint8_t) (prog < 0.5f ? 1 : (prog < 0.8f ? 2 : 4));
			if (g > 0)
				roll(D_SNARE);
			break;
		}
		case BT_STUTTER: {
			// the stab repeats faster and faster
			const Note* sm = p.mel[T_STAB - 1];
			const Note* found = nullptr;
			for (int i = 0; i < p.trackLen(T_STAB) && !found; i++)
				if (sm[i].on)
					found = &sm[i];
			int every = prog < 0.5f ? 4 : (prog < 0.8f ? 2 : 1);
			if (found && sib % every == 0) {
				out.mel[T_STAB - 1].on = true;
				out.mel[T_STAB - 1].note = *found;
				out.mel[T_STAB - 1].note.len = 1;
				out.mel[T_STAB - 1].note.ratch = (uint8_t) (prog > 0.9f ? 2 : 1);
			}
			if (g > 0)
				roll(D_CLAP);
			break;
		}
		case BT_STRIP:
			// elements go one by one
			if (prog > 0.25f)
				out.drum[D_PERC].vel = out.drum[D_RIM].vel = 0;
			if (prog > 0.5f)
				out.mel[T_LEAD - 1].on = false;
			if (prog > 0.75f)
				out.drum[D_KICK].vel = out.drum[D_OH].vel = 0;
			break;
		case BT_POLY:
			if (((c.secBar * 16 + sib) % 3) == 0)
				hit(D_PERC, 50 + (int) (70 * prog));
			break;
		case BT_CUT:
			if (lastBar && sib >= 8) {
				for (int v = 0; v < DRUMS; v++)
					out.drum[v].vel = 0;
				for (int t = 0; t < MELS; t++)
					out.mel[t].on = false;
			}
			break;
		default: break;
		}
		// before the drop: the kick off on the last beat, with GROW the last two 16ths silent
		if (lastBar && sib >= 12)
			out.drum[D_KICK].vel = 0;
		if (lastBar && g > 0 && sib >= 14) {
			for (int v = 0; v < DRUMS; v++)
				out.drum[v].vel = 0;
			for (int t = 0; t < MELS; t++)
				out.mel[t].on = false;
		}
	}
	// ---- the first 16th of a drop: an accent on the open hat (the impact)
	if (c.part == P_DROP && c.secBar == 0 && sib == 0)
		hit(D_OH, 127);
	// ---- GROW in later drops: more percussion, the lead an octave up (melodic styles)
	if (c.part == P_DROP && c.dropIdx > 0 && c.grow > 0) {
		if (sib % 8 == 6 && !out.drum[D_PERC].vel)
			hit(D_PERC, 72);
		if ((c.secBar & 1) && sib == 14)
			out.drum[D_CH].ratch = 2;
		if (s.family == F_MELODIC || s.family == F_TRANCE || s.family == F_DETROIT)
			out.mel[T_LEAD - 1].octave = 1;
	}
	// ---- fills at the end of a phrase (not in builds)
	bool phraseEnd = (c.secBar % 8) == 7 || lastBar;
	if (!build && (c.fill || (phraseEnd && (lastBar || ((c.secBar + 1) % 16) == 0)) ) && c.part != P_BREAK && c.part != P_INTRO) {
		Rng fr(hash(c.rnd, (uint32_t) (c.songStep / 16), 99));
		int kind = fr.range(3);
		if (sib >= 12) {
			if (kind == 0 && sib >= 13)
				hit(D_SNARE, 70 + (sib - 13) * 20);
			else if (kind == 1)
				hit(D_CH, 70 + (sib - 12) * 12, 2);
			else if (kind == 2 && (sib == 13 || sib == 15))
				hit(D_TOM, 95);
			if (lastBar && (c.nextPart == P_DROP) && sib >= 12)
				out.drum[D_KICK].vel = 0;
		}
	}
	// ---- the break before the build: a snare roll in the last bar when no build follows
	if (c.part == P_BREAK && lastBar && c.nextPart == P_DROP && sib % 2 == 0)
		hit(D_SNARE, 60 + sib * 4);
}

// ------------------------------------------------------------------ MIDI
static void putVar(std::vector<uint8_t>& d, uint32_t v) {
	uint8_t buf[5];
	int n = 0;
	buf[n++] = v & 0x7f;
	while ((v >>= 7))
		buf[n++] = 0x80 | (v & 0x7f);
	while (n)
		d.push_back(buf[--n]);
}
static void put32(std::vector<uint8_t>& d, uint32_t v) {
	for (int i = 3; i >= 0; i--)
		d.push_back((v >> (8 * i)) & 0xff);
}

std::vector<uint8_t> writeMidi(const std::vector<std::vector<MidiEv>>& tracks, const std::vector<std::string>& names,
                               float bpm, int ppq) {
	std::vector<uint8_t> d = {'M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 1};
	int nt = (int) tracks.size() + 1;
	d.push_back((nt >> 8) & 0xff);
	d.push_back(nt & 0xff);
	d.push_back((ppq >> 8) & 0xff);
	d.push_back(ppq & 0xff);
	// tempo track
	{
		std::vector<uint8_t> t;
		uint32_t us = (uint32_t) (60000000.0 / std::max(20.f, bpm));
		t.insert(t.end(), {0, 0xff, 0x51, 3, (uint8_t) (us >> 16), (uint8_t) (us >> 8), (uint8_t) us});
		t.insert(t.end(), {0, 0xff, 0x58, 4, 4, 2, 24, 8});
		t.insert(t.end(), {0, 0xff, 0x2f, 0});
		d.insert(d.end(), {'M', 'T', 'r', 'k'});
		put32(d, (uint32_t) t.size());
		d.insert(d.end(), t.begin(), t.end());
	}
	for (size_t k = 0; k < tracks.size(); k++) {
		std::vector<MidiEv> ev = tracks[k];
		std::stable_sort(ev.begin(), ev.end(), [](const MidiEv& a, const MidiEv& b) {
			if (a.tick != b.tick)
				return a.tick < b.tick;
			return (a.status & 0xf0) == 0x80 && (b.status & 0xf0) != 0x80; // offs first
		});
		std::vector<uint8_t> t;
		std::string nm = k < names.size() ? names[k] : "";
		t.insert(t.end(), {0, 0xff, 0x03});
		putVar(t, (uint32_t) nm.size());
		t.insert(t.end(), nm.begin(), nm.end());
		int64_t now = 0;
		for (const MidiEv& e : ev) {
			putVar(t, (uint32_t) std::max<int64_t>(0, e.tick - now));
			now = std::max(now, e.tick);
			t.push_back(e.status);
			t.push_back(e.d1);
			t.push_back(e.d2);
		}
		t.insert(t.end(), {0, 0xff, 0x2f, 0});
		d.insert(d.end(), {'M', 'T', 'r', 'k'});
		put32(d, (uint32_t) t.size());
		d.insert(d.end(), t.begin(), t.end());
	}
	return d;
}

std::vector<uint8_t> songMidi(const std::vector<const Pattern*>& pats, const std::vector<Section>& songIn, float bpm,
                              float energy, bool djEdges, int onlyTrack) {
	const int PPQ = 96, T16 = PPQ / 4;
	std::vector<Section> song = songIn;
	if (song.empty() && !pats.empty()) {
		Section x;
		x.part = P_DROP;
		x.bars = pats[0]->bars ? pats[0]->bars : 4;
		song.push_back(x);
	}
	std::vector<std::vector<MidiEv>> tr(5);
	std::vector<std::string> names = {"Drums", "Bass", "Stab", "Lead", "Atmo"};
	int64_t step = 0;
	for (size_t si = 0; si < song.size(); si++) {
		const Section& x = song[si];
		const Pattern* p = pats[std::min<size_t>(x.pad, pats.size() - 1)];
		if (!p)
			continue;
		float swing = st(p->style).swing;
		for (int bar = 0; bar < x.bars; bar++)
			for (int sib = 0; sib < 16; sib++, step++) {
				Ctx c;
				c.part = x.part;
				c.secBar = bar;
				c.secBars = x.bars;
				c.build = x.build;
				c.grow = x.grow;
				c.dropIdx = x.dropIdx;
				c.nextPart = si + 1 < song.size() ? (int) song[si + 1].part : (int) P_OUTRO;
				c.djEdges = djEdges;
				c.energy = energy;
				c.songStep = step;
				c.rnd = 12345;
				StepOut o;
				resolveStep(*p, c, o);
				int64_t base = step * T16 + ((sib & 1) ? (int64_t) ((swing - 50.f) / 50.f * T16) : 0);
				if (onlyTrack < 0 || onlyTrack == T_DRUMS)
					for (int v = 0; v < DRUMS; v++) {
						const DrumFire& f = o.drum[v];
						if (!f.vel)
							continue;
						int n = std::max(1, (int) f.ratch);
						for (int k = 0; k < n; k++) {
							int64_t at = base + f.micro * T16 / 100 + k * T16 / n;
							tr[0].push_back({std::max<int64_t>(0, at), 0x99, GM_DRUM[v], f.vel});
							tr[0].push_back({std::max<int64_t>(0, at) + T16 / n / 2 + 1, 0x89, GM_DRUM[v], 0});
						}
					}
				for (int t = 0; t < MELS; t++) {
					if (onlyTrack >= 0 && onlyTrack != t + 1)
						continue;
					const NoteFire& f = o.mel[t];
					if (!f.on)
						continue;
					int n = std::max(1, (int) f.note.ratch);
					int len = std::max(1, (int) f.note.len) * T16;
					for (int k = 0; k < n; k++) {
						int64_t at = std::max<int64_t>(0, base + f.note.micro * T16 / 100 + k * len / n);
						int64_t off = at + (n > 1 ? len / n : len) - (f.note.slide ? -4 : 4);
						for (int vv = 0; vv < std::max(1, (int) f.note.n); vv++) {
							int pitch = std::max(0, std::min(127, f.note.pitch[vv] + 12 * f.octave));
							tr[t + 1].push_back({at, (uint8_t) (0x90 | t), (uint8_t) pitch, f.note.vel});
							tr[t + 1].push_back({off, (uint8_t) (0x80 | t), (uint8_t) pitch, 0});
						}
					}
				}
			}
	}
	if (onlyTrack >= 0) {
		std::vector<std::vector<MidiEv>> one = {tr[onlyTrack]};
		std::vector<std::string> nm = {names[onlyTrack]};
		return writeMidi(one, nm, bpm, PPQ);
	}
	return writeMidi(tr, names, bpm, PPQ);
}

} // namespace tg
