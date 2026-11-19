// SPDX-License-Identifier: GPL-3.0-only
// GENRA TECHNO MOD: four CV lanes that move over the song. For every part of the song (INTRO … OUTRO) and every lane
// a movement (low, mid, high, rise, fall, swell, fade, wave, pump, wander, steps, jump, dip, off); DEPTH, SMOOTH and
// BARS per lane, TARGET only names it. It follows the song GENRA TECHNO plays: ABLAUF's when ABLAUF is connected, else
// GENRA TECHNO's own LIVE song; without a GENRA TECHNO it counts its CLK input and takes the part from PART (0..5 V).
// GROW makes later drops bigger, CHORD adds a little lift on every chord change, FREEZE holds the outputs.
// The matrix shows the song's sections side by side (INTRO, GROOVE, BUILD 1, DROP 1 …): a cell takes its part's
// movement unless it is set for that one section. Movements from outside: IN (the lane's jack IN A–D), IN SEC (the
// section's jack: INTRO, GROOVE 1/2, BUILD 1/2, DROP 1/2, BREAK 1/2, OUTRO; a third one takes the jack "2") and both
// multiplied.
#include "TechnoModule.hpp"
#include "TechnoUI.hpp"
#include "TechnoModLayout.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <array>

using namespace tg;
namespace L = technomod_layout;

// the buttons MIDI-MAP can press (BTN_PARAM + i; only ever append)
static const tui::BtnDef BTNS[] = {
    {L::BTN_UNI0, "UNI/BI A"},
    {L::BTN_UNI1, "UNI/BI B"},
    {L::BTN_UNI2, "UNI/BI C"},
    {L::BTN_UNI3, "UNI/BI D"},
    {L::BTN_GROW, "GROW"},
    {L::BTN_CHORD, "CHORD"},
    {L::BTN_FREEZE, "FREEZE"},
};
static const int NUM_BTNS = (int) (sizeof(BTNS) / sizeof(BTNS[0]));
static const tui::BtnDef FLDS[] = {
    {L::F_TARGET0, "TARGET A"}, {L::F_TARGET1, "TARGET B"}, {L::F_TARGET2, "TARGET C"}, {L::F_TARGET3, "TARGET D"},
    {L::F_BARS0, "BARS A"}, {L::F_BARS1, "BARS B"}, {L::F_BARS2, "BARS C"}, {L::F_BARS3, "BARS D"},
};

enum Shape { SH_OFF, SH_LOW, SH_MID, SH_HIGH, SH_RISE, SH_FALL, SH_SWELL, SH_FADE, SH_WAVE, SH_PUMP, SH_WANDER, SH_STEPS, SH_JUMP, SH_DIP,
             SH_IN, SH_INSEC, SH_INMUL, NUM_SHAPES };
static const char* const SHAPE_NAMES[NUM_SHAPES] = {"OFF",  "LOW",  "MID",    "HIGH",  "RISE", "FALL", "SWELL",
                                                    "FADE", "WAVE", "PUMP",   "WANDER", "STEPS", "JUMP", "DIP",
                                                    "IN",   "IN SEC", "IN x SEC"};
static bool isIn(int s) { return s >= SH_IN && s <= SH_INMUL; }
// the section jacks: INTRO, GROOVE 1, GROOVE 2, BUILD 1, BUILD 2, DROP 1, DROP 2, BREAK 1, BREAK 2, OUTRO
static const int NUM_SEC_IN = 10;
static const char* const SEC_IN_NAMES[NUM_SEC_IN] = {"INTRO", "GROOVE 1", "GROOVE 2", "BUILD 1", "BUILD 2",
                                                     "DROP 1", "DROP 2", "BREAK 1", "BREAK 2", "OUTRO"};
static const char* const SEC_IN_SHORT[NUM_SEC_IN] = {"INTRO", "GRV 1", "GRV 2", "BLD 1", "BLD 2",
                                                     "DROP 1", "DROP 2", "BRK 1", "BRK 2", "OUTRO"};
/** The jack of the k-th section (0: the first) of a part. */
static int secJackOf(int part, int k) {
	k = std::min(k, 1);
	switch (part) {
	case P_INTRO: return 0;
	case P_GROOVE: return 1 + k;
	case P_BUILD: return 3 + k;
	case P_DROP: return 5 + k;
	case P_BREAK: return 7 + k;
	default: return 9;
	}
}
static const int MAX_SECS = 128;
static const char* const TARGETS[8] = {"CUTOFF", "RESO", "SEND", "LEVEL", "DRIVE", "PAN", "DECAY", "FREE"};
static const int BARS_OF[6] = {1, 2, 4, 8, 16, 32};

struct ModPreset {
	const char* name;
	int shape[6][4];
	int target[4];
	int bars[4];
};
static const ModPreset PRESETS[6] = {
    {"PEAK TIME SWEEPS",
     {{SH_LOW, SH_LOW, SH_OFF, SH_LOW}, {SH_WAVE, SH_LOW, SH_WAVE, SH_OFF}, {SH_RISE, SH_SWELL, SH_WAVE, SH_HIGH},
      {SH_JUMP, SH_MID, SH_HIGH, SH_OFF}, {SH_FALL, SH_HIGH, SH_STEPS, SH_HIGH}, {SH_FALL, SH_FALL, SH_LOW, SH_FALL}},
     {0, 2, 3, 7}, {4, 4, 3, 4}},
    {"DUB THROWS",
     {{SH_LOW, SH_MID, SH_OFF, SH_LOW}, {SH_WANDER, SH_STEPS, SH_WAVE, SH_LOW}, {SH_RISE, SH_SWELL, SH_RISE, SH_MID},
      {SH_MID, SH_STEPS, SH_WAVE, SH_LOW}, {SH_WANDER, SH_HIGH, SH_WANDER, SH_HIGH}, {SH_FALL, SH_HIGH, SH_FALL, SH_FALL}},
     {0, 2, 6, 7}, {3, 1, 4, 4}},
    {"HYPNOTIC DRIFT",
     {{SH_WANDER, SH_LOW, SH_WAVE, SH_OFF}, {SH_WANDER, SH_WAVE, SH_WAVE, SH_WANDER}, {SH_SWELL, SH_RISE, SH_WAVE, SH_WANDER},
      {SH_WANDER, SH_WAVE, SH_WAVE, SH_WANDER}, {SH_WAVE, SH_HIGH, SH_WANDER, SH_LOW}, {SH_FADE, SH_FALL, SH_WAVE, SH_OFF}},
     {0, 2, 1, 5}, {5, 4, 3, 5}},
    {"TRANCE LIFT",
     {{SH_LOW, SH_LOW, SH_MID, SH_OFF}, {SH_MID, SH_LOW, SH_MID, SH_WAVE}, {SH_SWELL, SH_RISE, SH_RISE, SH_SWELL},
      {SH_HIGH, SH_MID, SH_HIGH, SH_WAVE}, {SH_LOW, SH_HIGH, SH_MID, SH_WANDER}, {SH_FALL, SH_FALL, SH_FALL, SH_OFF}},
     {0, 2, 3, 7}, {4, 4, 4, 3}},
    {"PUMP",
     {{SH_OFF, SH_OFF, SH_OFF, SH_OFF}, {SH_PUMP, SH_PUMP, SH_WAVE, SH_OFF}, {SH_RISE, SH_PUMP, SH_RISE, SH_OFF},
      {SH_PUMP, SH_PUMP, SH_HIGH, SH_OFF}, {SH_OFF, SH_HIGH, SH_WANDER, SH_OFF}, {SH_PUMP, SH_FALL, SH_FALL, SH_OFF}},
     {3, 2, 0, 7}, {0, 0, 3, 3}},
    {"CLEAN (ALL OFF)",
     {{SH_OFF, SH_OFF, SH_OFF, SH_OFF}, {SH_OFF, SH_OFF, SH_OFF, SH_OFF}, {SH_OFF, SH_OFF, SH_OFF, SH_OFF},
      {SH_OFF, SH_OFF, SH_OFF, SH_OFF}, {SH_OFF, SH_OFF, SH_OFF, SH_OFF}, {SH_OFF, SH_OFF, SH_OFF, SH_OFF}},
     {0, 2, 3, 7}, {3, 3, 3, 3}},
};

/** The value of a movement (0..1). t: position in the part (0..1), lastBar: position in the part's last bar (or -1),
 * wave: phase over BARS (any real), beat: position in the beat (0..1), bar: bar counter (for the random ones). */
static float shapeValue(int s, float t, float lastBar, double wave, float beat, int64_t bar, int beatNo, int lane) {
	(void) wave;
	switch (s) {
	case SH_LOW: return 0.15f;
	case SH_MID: return 0.5f;
	case SH_HIGH: return 0.85f;
	case SH_RISE: return t;
	case SH_FALL: return 1.f - t;
	case SH_SWELL: return std::pow(t, 2.5f);
	case SH_FADE: return std::pow(1.f - t, 2.5f);
	case SH_WAVE: return 0.5f + 0.45f * (float) std::sin(2.0 * M_PI * wave);
	case SH_PUMP: return 1.f - beat;
	case SH_WANDER: {
		// a new random point every bar, a smooth way between them
		float a = (hash((uint32_t) bar, lane, 7) % 1000) / 1000.f;
		float b = (hash((uint32_t) (bar + 1), lane, 7) % 1000) / 1000.f;
		float frac = (((beatNo % 4) + 4) % 4 + beat) / 4.f;
		float f = 0.5f - 0.5f * std::cos((float) M_PI * frac);
		return 0.2f + 0.6f * (a + (b - a) * f);
	}
	case SH_STEPS: return 0.1f + 0.8f * ((hash((uint32_t) beatNo, (uint32_t) bar, lane + 3) % 1000) / 1000.f);
	case SH_JUMP: return 0.6f + 0.4f * std::pow(1.f - t, 3.f);
	case SH_DIP: return lastBar >= 0.f ? 0.85f * (1.f - lastBar) : 0.85f;
	default: return 0.f;
	}
}

struct TechnoMod : Module {
	enum ParamId { ENUMS(DEPTH_PARAM, 4), ENUMS(SMOOTH_PARAM, 4), ENUMS(BTN_PARAM, 7), PRESETSEL_PARAM, ENUMS(FLD_PARAM, 8), NUM_PARAMS };
	enum InputId { CLK_INPUT, RST_INPUT, PART_INPUT, DEPTH_INPUT, ENUMS(LANEIN_INPUT, 4), ENUMS(SEC_INPUT, NUM_SEC_IN), NUM_INPUTS };
	enum OutputId { ENUMS(LANE_OUTPUT, 4), PHRASE_OUTPUT, PART_OUTPUT, BAR_OUTPUT, NUM_OUTPUTS };
	TechnoLink link;
	int shape[6][4];
	int target[4], bars[4];
	bool uni[4] = {true, true, true, true};
	bool grow = true, chord = true, freeze = false;
	int preset = 0;
	// audio
	dsp::SchmittTrigger clkTrig, rstTrig;
	dsp::PulseGenerator phraseP, barP;
	int64_t ownStep = -1;
	float sinceClk = 0.f, clkGap = 5000.f;
	int64_t lastBarCount = -1;
	int lastChord = 0;
	float bump[4] = {};
	float y[4] = {};
	int ctl = 0;
	// published for the panel
	std::atomic<int> pubPart{P_GROOVE};
	std::atomic<bool> pubOwn{true};
	std::atomic<int> pubSec{-1};    // the section that plays (-1: none known)
	std::atomic<float> extNow[4];   // the value from outside a lane uses now (0..1), -1: none
	// the song's sections (UI thread): a cell set for one section (part: the part it was set for; another part at that
	// place after a new song: not used); and the table the audio thread reads (written by the panel)
	struct SecOv {
		int8_t part = -1;
		int8_t shape[4] = {-1, -1, -1, -1};
	};
	SecOv ov[MAX_SECS];
	int8_t effShape[MAX_SECS][4] = {};
	int8_t effPart[MAX_SECS] = {};
	int8_t secJack[MAX_SECS] = {};
	std::atomic<int> effN{0};

	TechnoMod() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, 0);
		static_assert(sizeof(BTNS) / sizeof(BTNS[0]) == 7, "BTN_PARAM count");
		tui::configButtons(this, BTN_PARAM, BTNS, NUM_BTNS);
		configParam(PRESETSEL_PARAM, 0.f, 1.f, 0.f, "Preset select (for MIDI-MAP: a knob goes through the presets)", " %", 0.f, 100.f);
		paramQuantities[PRESETSEL_PARAM]->randomizeEnabled = false;
		paramQuantities[PRESETSEL_PARAM]->resetEnabled = false;
		tui::configSelects(this, FLD_PARAM, FLDS, 8);
		for (int j = 0; j < 4; j++) {
			std::string n(1, (char) ('A' + j));
			configParam(DEPTH_PARAM + j, 0.f, 1.f, 0.8f, "Lane " + n + " depth", " %", 0.f, 100.f);
			configParam(SMOOTH_PARAM + j, 0.f, 1.f, 0.3f, "Lane " + n + " smooth (glide between the parts)", " %", 0.f, 100.f);
			configOutput(LANE_OUTPUT + j, "Lane " + n + " (UNI 0..10 V, BI ±5 V)");
			configInput(LANEIN_INPUT + j, "Lane " + n + " from outside (IN; UNI 0..10 V, BI ±5 V)");
			extNow[j] = -1.f;
		}
		for (int k = 0; k < NUM_SEC_IN; k++)
			configInput(SEC_INPUT + k, std::string(SEC_IN_NAMES[k]) + " from outside (IN SEC)");
		configInput(CLK_INPUT, "Clock (16ths; only without a GENRA TECHNO)");
		configInput(RST_INPUT, "Reset (only without a GENRA TECHNO)");
		configInput(PART_INPUT, "Part (0 V intro … 5 V outro; only without a GENRA TECHNO)");
		configInput(DEPTH_INPUT, "Depth CV (±10 V: ±100 % on all lanes)");
		configOutput(PHRASE_OUTPUT, "Phrase (a trigger every 8 bars)");
		configOutput(PART_OUTPUT, "Part out (0 V intro … 5 V outro)");
		configOutput(BAR_OUTPUT, "Bar (trigger)");
		link.owner = this;
		loadPreset(0);
	}
	void loadPreset(int p) {
		preset = clamp(p, 0, 5);
		for (int i = 0; i < MAX_SECS; i++) // a preset sets every part anew: no section keeps its own
			ov[i] = SecOv();
		const ModPreset& m = PRESETS[preset];
		for (int i = 0; i < 6; i++)
			for (int j = 0; j < 4; j++)
				shape[i][j] = m.shape[i][j];
		for (int j = 0; j < 4; j++) {
			target[j] = m.target[j];
			bars[j] = m.bars[j];
		}
	}

	/** Where the song is: part, position in the part, bars … (from GENRA TECHNO or the own clock). */
	struct Pos {
		int part = P_GROOVE, secBar = 0, secBars = 8, dropIdx = 0;
		float t = 0.f, beat = 0.f;
		double step = 0; // 16ths, with the fraction
		int64_t bar = 0;
		bool running = false;
		int sec = -1; // the section in the song (-1: none, the part's movement)
	};
	Pos where() {
		Pos p;
		GenraTechno* g = link.main.load();
		if (g && g->running.load()) {
			p.running = true;
			p.part = g->pubPart.load();
			p.secBar = g->pubSecBar.load();
			p.secBars = std::max(1, g->pubSecBars.load());
			p.dropIdx = g->pubDropIdx.load();
			int sib = g->pubStepInBar.load();
			p.step = (double) g->pubSongStep.load();
			p.t = clamp((p.secBar * 16 + sib) / (float) (p.secBars * 16), 0.f, 1.f);
			p.beat = (sib % 4) / 4.f;
			p.bar = g->pubBarCount.load();
			p.sec = g->pubSecIdx.load();
			return p;
		}
		if (ownStep >= 0) {
			p.running = true;
			float frac = std::min(0.999f, sinceClk / std::max(1.f, clkGap));
			p.step = ownStep + frac;
			p.bar = ownStep / 16;
			p.secBar = (int) (p.bar % 8);
			p.secBars = 8;
			p.t = (float) (std::fmod(p.step, 128.0) / 128.0);
			p.beat = (float) std::fmod(p.step / 4.0, 1.0);
			p.part = inputs[PART_INPUT].isConnected() ? clamp((int) std::round(inputs[PART_INPUT].getVoltage()), 0, 5) : P_GROOVE;
		}
		return p;
	}

	/** The movement of lane j in a section (or, without one, the part's) and the section's jack. */
	int shapeAt(int j, const Pos& p, int* jack) {
		int n = effN.load();
		if (p.sec >= 0 && p.sec < n && effPart[p.sec] == p.part) {
			*jack = secJack[p.sec];
			return effShape[p.sec][j];
		}
		*jack = secJackOf(p.part, 0);
		return shape[p.part % 6][j];
	}
	/** A jack's voltage as a movement value (0..1): UNI 0..10 V, BI ±5 V; unplugged: rest (0 or the middle). */
	float fromOutside(int input, int j) {
		if (!inputs[input].isConnected())
			return uni[j] ? 0.f : 0.5f;
		float v = inputs[input].getVoltage();
		return clamp(uni[j] ? v / 10.f : v / 10.f + 0.5f, 0.f, 1.f);
	}
	float laneValue(int j, const Pos& p) {
		int jack = 0;
		int s = shapeAt(j, p, &jack);
		if (isIn(s)) {
			float a = fromOutside(LANEIN_INPUT + j, j), b = fromOutside(SEC_INPUT + jack, j);
			float v = s == SH_IN ? a : (s == SH_INSEC ? b : a * b);
			extNow[j] = v;
			return v;
		}
		extNow[j] = -1.f;
		float lastBar = p.secBar == p.secBars - 1 ? (float) std::fmod(p.step, 16.0) / 16.f : -1.f;
		double wave = p.step / (16.0 * BARS_OF[bars[j] % 6]);
		int beatNo = (int) (p.step / 4.0);
		return shapeValue(s, p.t, lastBar, wave, p.beat, p.bar, beatNo, j);
	}

	void process(const ProcessArgs& args) override {
		GenraTechno* g = link.main.load();
		pubOwn = g == nullptr;
		// the own clock (without a GENRA TECHNO)
		sinceClk += 1.f;
		if (rstTrig.process(inputs[RST_INPUT].getVoltage(), 0.1f, 1.f))
			ownStep = -1;
		if (clkTrig.process(inputs[CLK_INPUT].getVoltage(), 0.1f, 1.f)) {
			if (sinceClk > 10.f && ownStep >= 0)
				clkGap = 0.7f * clkGap + 0.3f * sinceClk;
			sinceClk = 0.f;
			ownStep++;
			if (!g && ownStep % 16 == 0) {
				barP.trigger(1e-3f);
				if (ownStep % 128 == 0)
					phraseP.trigger(1e-3f);
			}
		}
		if (g) {
			int64_t bc = g->pubBarCount.load();
			if (bc != lastBarCount && g->running.load()) {
				barP.trigger(1e-3f);
				if (g->pubSecBar.load() % 8 == 0)
					phraseP.trigger(1e-3f);
			}
			lastBarCount = bc;
			int cc = g->pubChordCount.load();
			if (cc != lastChord && chord)
				for (int j = 0; j < 4; j++)
					bump[j] = 0.15f;
			lastChord = cc;
		}
		// the lanes, every 16 samples
		if (++ctl >= 16) {
			ctl = 0;
			float dt = 16.f * args.sampleTime;
			Pos p = where();
			pubPart = p.part;
			pubSec = p.sec;
			float dcv = clamp(inputs[DEPTH_INPUT].getVoltage() / 10.f, -1.f, 1.f);
			float growMul = (grow && p.part == P_DROP) ? 1.f + 0.25f * std::min(2, p.dropIdx) : 1.f;
			for (int j = 0; j < 4; j++) {
				bump[j] *= std::exp(-dt / 0.12f);
				if (freeze || !p.running)
					continue;
				int jk = 0;
				int s = shapeAt(j, p, &jk);
				float v = laneValue(j, p);
				float d = clamp(params[DEPTH_PARAM + j].getValue() + dcv, 0.f, 1.f) * growMul;
				float target;
				if (s == SH_OFF)
					target = uni[j] ? 0.f : 0.f;
				else if (uni[j])
					target = 10.f * clamp((v + bump[j]) * d, 0.f, 1.f);
				else
					target = 5.f * clamp((v + bump[j] - 0.5f) * 2.f * d, -1.f, 1.f);
				float sm = params[SMOOTH_PARAM + j].getValue();
				float tau = 0.002f + sm * sm * 2.f;
				y[j] += (target - y[j]) * (1.f - std::exp(-dt / tau));
			}
		}
		for (int j = 0; j < 4; j++)
			outputs[LANE_OUTPUT + j].setVoltage(y[j]);
		outputs[PART_OUTPUT].setVoltage((float) pubPart.load());
		outputs[PHRASE_OUTPUT].setVoltage(phraseP.process(args.sampleTime) ? 10.f : 0.f);
		outputs[BAR_OUTPUT].setVoltage(barP.process(args.sampleTime) ? 10.f : 0.f);
	}

	json_t* dataToJson() override {
		json_t* root = json_object();
		link.toJson(root);
		json_t* m = json_array();
		for (int i = 0; i < 6; i++)
			for (int j = 0; j < 4; j++)
				json_array_append_new(m, json_integer(shape[i][j]));
		json_object_set_new(root, "matrix", m);
		json_t* l = json_array();
		for (int j = 0; j < 4; j++) {
			json_t* o = json_object();
			json_object_set_new(o, "target", json_integer(target[j]));
			json_object_set_new(o, "bars", json_integer(bars[j]));
			json_object_set_new(o, "uni", json_boolean(uni[j]));
			json_array_append_new(l, o);
		}
		json_object_set_new(root, "lanes", l);
		json_object_set_new(root, "grow", json_boolean(grow));
		json_object_set_new(root, "chord", json_boolean(chord));
		json_object_set_new(root, "freeze", json_boolean(freeze));
		json_object_set_new(root, "preset", json_integer(preset));
		json_t* o = json_array(); // the cells set for one section
		for (int i = 0; i < MAX_SECS; i++)
			if (ov[i].part >= 0) {
				json_t* e = json_object();
				json_object_set_new(e, "i", json_integer(i));
				json_object_set_new(e, "part", json_integer(ov[i].part));
				json_t* sh = json_array();
				for (int j = 0; j < 4; j++)
					json_array_append_new(sh, json_integer(ov[i].shape[j]));
				json_object_set_new(e, "shape", sh);
				json_array_append_new(o, e);
			}
		json_object_set_new(root, "sections", o);
		return root;
	}
	uint32_t jsonCode = 0; // bumped by dataFromJson: the preset select (MIDI-MAP) does not load then
	void dataFromJson(json_t* root) override {
		jsonCode++;
		link.fromJson(root);
		if (json_t* m = json_object_get(root, "matrix"))
			for (int k = 0; k < 24 && k < (int) json_array_size(m); k++)
				shape[k / 4][k % 4] = clamp((int) json_integer_value(json_array_get(m, k)), 0, NUM_SHAPES - 1);
		if (json_t* l = json_object_get(root, "lanes"))
			for (int j = 0; j < 4 && j < (int) json_array_size(l); j++) {
				json_t* o = json_array_get(l, j);
				target[j] = clamp((int) json_integer_value(json_object_get(o, "target")), 0, 7);
				bars[j] = clamp((int) json_integer_value(json_object_get(o, "bars")), 0, 5);
				uni[j] = !json_is_false(json_object_get(o, "uni"));
			}
		grow = !json_is_false(json_object_get(root, "grow"));
		chord = !json_is_false(json_object_get(root, "chord"));
		freeze = json_is_true(json_object_get(root, "freeze"));
		if (json_t* p = json_object_get(root, "preset"))
			preset = clamp((int) json_integer_value(p), 0, 5);
		for (int i = 0; i < MAX_SECS; i++)
			ov[i] = SecOv();
		if (json_t* o = json_object_get(root, "sections"))
			for (size_t k = 0; k < json_array_size(o); k++) {
				json_t* e = json_array_get(o, k);
				int i = (int) json_integer_value(json_object_get(e, "i"));
				if (i < 0 || i >= MAX_SECS)
					continue;
				ov[i].part = (int8_t) clamp((int) json_integer_value(json_object_get(e, "part")), 0, NUM_PARTS - 1);
				json_t* sh = json_object_get(e, "shape");
				for (int j = 0; j < 4 && j < (int) json_array_size(sh); j++)
					ov[i].shape[j] = (int8_t) clamp((int) json_integer_value(json_array_get(sh, j)), -1, NUM_SHAPES - 1);
			}
	}
	void onReset() override {
		Module::onReset();
		loadPreset(0);
		for (int i = 0; i < MAX_SECS; i++)
			ov[i] = SecOv();
	}
};

// ================================================================ the panel
/** The song's sections for the matrix (UI thread): from GENRA TECHNO's song, or the six parts without one. */
struct Cols {
	std::vector<Section> secs;
	bool song = false;           // false: the six parts (no song known)
	std::vector<int> jack;       // the section jack of every column
	std::vector<std::string> name;
};
static Cols columnsOf(GenraTechno* g) {
	Cols c;
	if (g) {
		std::lock_guard<std::mutex> lk(g->viewMx);
		c.secs = g->view.sections;
	}
	if ((int) c.secs.size() > MAX_SECS)
		c.secs.resize(MAX_SECS);
	c.song = !c.secs.empty();
	if (!c.song)
		for (int i = 0; i < NUM_PARTS; i++) {
			Section s;
			s.part = (uint8_t) i;
			s.bars = 8;
			c.secs.push_back(s);
		}
	int seen[NUM_PARTS] = {}, total[NUM_PARTS] = {};
	for (const Section& s : c.secs)
		total[s.part % NUM_PARTS]++;
	for (const Section& s : c.secs) {
		int part = s.part % NUM_PARTS;
		int k = seen[part]++;
		c.jack.push_back(secJackOf(part, k));
		c.name.push_back(c.song && total[part] > 1 ? string::f("%s %d", PART_NAMES[part], k + 1) : std::string(PART_NAMES[part]));
	}
	return c;
}

struct MatrixView : widget::OpaqueWidget {
	TechnoMod* mod = nullptr;
	std::function<GenraTechno*()> gm;
	Cols cols;
	int first = 0;          // the first column shown
	bool userPaged = false; // paged by hand: no following the playhead until it leaves the page
	static const int SHOWN = 9;
	float headH() { return mm2px(3.6f); }
	float rowTop() { return mm2px(5.6f); }
	float rowH() { return mm2px(7.6f); }
	float rowGap() { return mm2px(0.8f); }
	float colW() { return box.size.x / SHOWN; }
	int nCols() { return (int) cols.secs.size(); }

	/** The movement of a cell (lane j, column i) and whether it is set for this one section. */
	int cell(int i, int j, bool* own) {
		const Section& s = cols.secs[i];
		int part = s.part % NUM_PARTS;
		*own = false;
		if (cols.song && i < MAX_SECS && mod->ov[i].part == part && mod->ov[i].shape[j] >= 0) {
			*own = true;
			return mod->ov[i].shape[j];
		}
		return mod->shape[part][j];
	}
	void set(int i, int j, int s) {
		const Section& sec = cols.secs[i];
		int part = sec.part % NUM_PARTS;
		if (!cols.song) { // no song: the part itself
			mod->shape[part][j] = s;
			return;
		}
		TechnoMod::SecOv& o = mod->ov[i];
		if (o.part != part) {
			o = TechnoMod::SecOv();
			o.part = (int8_t) part;
		}
		o.shape[j] = (int8_t) s;
	}
	void fromPart(int i, int j) {
		if (i < MAX_SECS && cols.song)
			mod->ov[i].shape[j] = -1;
	}
	void forEvery(int i, int j, int s) { // the part's movement, and no section of that part keeps its own
		int part = cols.secs[i].part % NUM_PARTS;
		mod->shape[part][j] = s;
		for (int k = 0; k < nCols() && k < MAX_SECS; k++)
			if (cols.secs[k].part % NUM_PARTS == part && mod->ov[k].part == part)
				mod->ov[k].shape[j] = -1;
	}
	std::string inName(int s, int i, int j) {
		std::string sec = SEC_IN_SHORT[cols.jack[i]];
		std::string lane(1, (char) ('A' + j));
		if (s == SH_IN)
			return "IN " + lane;
		if (s == SH_INSEC)
			return "IN " + sec;
		return lane + " x " + sec;
	}

	void step() override {
		OpaqueWidget::step();
		if (!mod)
			return;
		cols = columnsOf(gm ? gm() : nullptr);
		first = clamp(first, 0, std::max(0, nCols() - SHOWN));
		// follow the playhead (unless paged by hand and it is still on the page)
		int cur = mod->pubSec.load();
		if (cols.song && cur >= 0 && cur < nCols()) {
			bool onPage = cur >= first && cur < first + SHOWN;
			if (!onPage) {
				userPaged = false;
				first = clamp(cur - 1, 0, std::max(0, nCols() - SHOWN));
			}
		}
	}
	void draw(const DrawArgs& args) override {
		using namespace tui;
		if (!mod)
			return;
		NVGcontext* vg = args.vg;
		auto f = bold();
		int cur = mod->pubSec.load(), curPart = mod->pubPart.load();
		float cw = colW();
		for (int c = 0; c < SHOWN; c++) {
			int i = first + c;
			if (i >= nCols())
				break;
			const Section& s = cols.secs[i];
			int part = s.part % NUM_PARTS;
			bool now = cols.song ? i == cur : part == curPart;
			float x = c * cw;
			roundRect(vg, x + 1, 0, cw - 2, headH(), 1.5f, now ? PART_COL[part] : dark(PART_COL[part], 0.55f));
			textFit(vg, f, x + cw / 2, headH() / 2 + 0.5f, cols.name[i], 5.6f, cw - 4, now ? BG : TXT, NVG_ALIGN_CENTER, 0.3f);
			for (int j = 0; j < 4; j++) {
				float y = rowTop() + j * (rowH() + rowGap());
				bool own = false;
				int sh = cell(i, j, &own);
				NVGcolor lc = LANE_COL[j];
				if (isIn(sh)) {
					roundRect(vg, x + 1, y, cw - 2, rowH(), 2.f, lc);
					textFit(vg, f, x + cw / 2, y + rowH() / 2 + 0.5f, inName(sh, i, j), 6.f, cw - 5, BG, NVG_ALIGN_CENTER, 0.3f);
				}
				else {
					roundRect(vg, x + 1, y, cw - 2, rowH(), 2.f, now ? dark(lc, 0.35f) : BTN);
					drawShape(vg, x + 4, y + 3, cw - 8, rowH() - 6, sh, sh == SH_OFF ? nvgRGB(0x44, 0x48, 0x52) : lc, j);
				}
				if (now)
					frame(vg, x + 1.5f, y + 0.5f, cw - 3, rowH() - 1, 2.f, isIn(sh) ? WHITE : lc, 1.f);
				if (own) { // set for this one section
					nvgBeginPath(vg);
					nvgCircle(vg, x + cw - 4.5f, y + 3.5f, 1.6f);
					nvgFillColor(vg, WHITE);
					nvgFill(vg);
				}
			}
		}
		// more columns than shown: small arrows at the head
		if (first > 0)
			text(vg, f, -2, headH() / 2 + 0.5f, "<", 7.f, DIM, NVG_ALIGN_RIGHT);
		if (first + SHOWN < nCols())
			text(vg, f, box.size.x + 2, headH() / 2 + 0.5f, ">", 7.f, DIM, NVG_ALIGN_LEFT);
	}
	static void drawShape(NVGcontext* vg, float x, float y, float w, float h, int s, NVGcolor c, int lane) {
		nvgBeginPath(vg);
		if (s == SH_OFF || isIn(s)) {
			for (float u = 0; u < w; u += 4) {
				nvgMoveTo(vg, x + u, y + h / 2);
				nvgLineTo(vg, x + std::min(w, u + 2), y + h / 2);
			}
		}
		else
			for (int i = 0; i <= 32; i++) {
				float t = i / 32.f;
				float lb = t > 0.75f ? (t - 0.75f) / 0.25f : -1.f;
				float v = shapeValue(s, t, lb, t * 2.0, std::fmod(t * 4.f, 1.f), 3, (int) (t * 8), lane);
				float px = x + t * w, py = y + h - v * h;
				if (i == 0)
					nvgMoveTo(vg, px, py);
				else
					nvgLineTo(vg, px, py);
			}
		nvgStrokeColor(vg, c);
		nvgStrokeWidth(vg, 1.2f);
		nvgLineJoin(vg, NVG_ROUND);
		nvgStroke(vg);
	}
	bool cellAt(Vec p, int* i, int* j) {
		if (p.y < rowTop())
			return false;
		int c = (int) (p.x / colW());
		*j = (int) ((p.y - rowTop()) / (rowH() + rowGap()));
		*i = first + c;
		return c >= 0 && c < SHOWN && *i < nCols() && *j >= 0 && *j < 4;
	}
	void openMenu(int i, int j) {
		ui::Menu* menu = createMenu();
		std::string part = PART_NAMES[cols.secs[i].part % NUM_PARTS];
		menu->addChild(createMenuLabel(string::f("%s · lane %c", cols.name[i].c_str(), 'A' + j)));
		bool own = false;
		int curS = cell(i, j, &own);
		MatrixView* mv = this;
		for (int s = 0; s < NUM_SHAPES; s++) {
			if (s == SH_IN)
				menu->addChild(new ui::MenuSeparator);
			std::string lb = isIn(s) ? inName(s, i, j) : std::string(SHAPE_NAMES[s]);
			std::string hint = s == SH_IN ? string::f("jack IN %c", 'A' + j)
			                              : (s == SH_INSEC ? std::string("jack ") + SEC_IN_NAMES[cols.jack[i]]
			                                               : (s == SH_INMUL ? std::string("both jacks multiplied") : ""));
			menu->addChild(createCheckMenuItem(lb, hint, [=]() { return curS == s; }, [=]() { mv->set(i, j, s); }));
		}
		if (cols.song) {
			menu->addChild(new ui::MenuSeparator);
			menu->addChild(createMenuItem(string::f("Like every %s", part.c_str()), own ? "" : "(already)",
			                              [=]() { mv->fromPart(i, j); }));
			menu->addChild(createMenuItem(string::f("This for every %s", part.c_str()), "", [=]() { mv->forEvery(i, j, curS); }));
		}
	}
	void onButton(const ButtonEvent& e) override {
		if (!mod || e.action != GLFW_PRESS)
			return;
		int i, j;
		if (!cellAt(e.pos, &i, &j))
			return;
		e.consume(this);
		if (e.button == GLFW_MOUSE_BUTTON_LEFT || e.button == GLFW_MOUSE_BUTTON_RIGHT)
			openMenu(i, j);
	}
	void onHoverScroll(const HoverScrollEvent& e) override {
		if (!mod)
			return;
		int d = e.scrollDelta.y > 0 ? -1 : (e.scrollDelta.y < 0 ? 1 : 0);
		if (e.pos.y < rowTop()) { // on the head: page through the sections
			first = clamp(first + d, 0, std::max(0, nCols() - SHOWN));
			userPaged = true;
			e.consume(this);
			return;
		}
		int i, j;
		if (!cellAt(e.pos, &i, &j))
			return;
		bool own = false;
		int s = cell(i, j, &own);
		set(i, j, clamp(s + d, 0, NUM_SHAPES - 1));
		e.consume(this);
	}
};

struct TechnoModWidget : ModuleWidget {
	TechnoMod* mod = nullptr;
	GenraTechno* gm() { return mod ? mod->link.main.load() : nullptr; }
	MatrixView* mv = nullptr;
	// what came in from outside, per section and lane (64 points over the section; -1: nothing yet), for the display
	std::vector<std::array<float, 4 * 64>> hist;
	uint32_t histSig = 0;
	bool secHere[NUM_SEC_IN] = {};
	std::string secTip[NUM_SEC_IN];

	bool btnsMapped = false;
	TechnoModWidget(TechnoMod* module) {
		setModule(module);
		mod = module;
		setPanel(createPanel(asset::plugin(pluginInstance, "res/TechnoMod.svg")));
		using namespace tui;
		static const float* const KD[4] = {L::K_DEPTH0, L::K_DEPTH1, L::K_DEPTH2, L::K_DEPTH3};
		static const float* const KS[4] = {L::K_SMOOTH0, L::K_SMOOTH1, L::K_SMOOTH2, L::K_SMOOTH3};
		static const float* const FT[4] = {L::F_TARGET0, L::F_TARGET1, L::F_TARGET2, L::F_TARGET3};
		static const float* const FB[4] = {L::F_BARS0, L::F_BARS1, L::F_BARS2, L::F_BARS3};
		static const float* const BU[4] = {L::BTN_UNI0, L::BTN_UNI1, L::BTN_UNI2, L::BTN_UNI3};
		static const float* const OUT[4] = {L::OUT_A, L::OUT_B, L::OUT_C, L::OUT_D};
		for (int j = 0; j < 4; j++) {
			addParam(createParamCentered<GKnob>(mm(KD[j]), module, TechnoMod::DEPTH_PARAM + j));
			addParam(createParamCentered<GSmallKnob>(mm(KS[j]), module, TechnoMod::SMOOTH_PARAM + j));
			addOutput(createOutputCentered<PJ301MPort>(mm(OUT[j]), module, TechnoMod::LANE_OUTPUT + j));
			TField* t = place<TField>(this, FT[j]);
			t->col = LANE_COL[j];
			t->value = [=]() { return std::string(TARGETS[mod ? mod->target[j] : 0]); };
			t->count = []() { return 8; };
			t->current = [=]() { return mod ? mod->target[j] : 0; };
			t->item = [](int i) { return std::string(TARGETS[i]); };
			t->choose = [=](int i) {
				if (mod)
					mod->target[j] = i;
			};
			TField* b = place<TField>(this, FB[j]);
			b->col = LANE_COL[j];
			b->value = [=]() { return string::f("%d", BARS_OF[mod ? mod->bars[j] : 3]); };
			b->count = []() { return 6; };
			b->current = [=]() { return mod ? mod->bars[j] : 3; };
			b->item = [](int i) { return string::f("%d bars (waves)", BARS_OF[i]); };
			b->choose = [=](int i) {
				if (mod)
					mod->bars[j] = i;
			};
			TButton* u = place<TButton>(this, BU[j]);
			u->labelOf = [=]() { return std::string(mod && !mod->uni[j] ? "BI" : "UNI"); };
			u->fontSize = 7.f;
			u->act = [=]() {
				if (mod)
					mod->uni[j] = !mod->uni[j];
			};
		}
		static const float* const IN[4] = {L::IN_CLK, L::IN_RST, L::IN_PART, L::IN_DEPTH};
		for (int i = 0; i < 4; i++)
			addInput(createInputCentered<PJ301MPort>(mm(IN[i]), module, i));
		static const float* const LIN[4] = {L::IN_A, L::IN_B, L::IN_C, L::IN_D};
		for (int j = 0; j < 4; j++)
			addInput(createInputCentered<PJ301MPort>(mm(LIN[j]), module, TechnoMod::LANEIN_INPUT + j));
		static const float* const SIN[NUM_SEC_IN] = {L::IN_SEC0, L::IN_SEC1, L::IN_SEC2, L::IN_SEC3, L::IN_SEC4,
		                                             L::IN_SEC5, L::IN_SEC6, L::IN_SEC7, L::IN_SEC8, L::IN_SEC9};
		static const float* const SLB[NUM_SEC_IN] = {L::A_SL0, L::A_SL1, L::A_SL2, L::A_SL3, L::A_SL4,
		                                             L::A_SL5, L::A_SL6, L::A_SL7, L::A_SL8, L::A_SL9};
		static const int SEC_PART[NUM_SEC_IN] = {P_INTRO, P_GROOVE, P_GROOVE, P_BUILD, P_BUILD, P_DROP, P_DROP, P_BREAK, P_BREAK, P_OUTRO};
		for (int k = 0; k < NUM_SEC_IN; k++) {
			addInput(createInputCentered<PJ301MPort>(mm(SIN[k]), module, TechnoMod::SEC_INPUT + k));
			// the name under the jack: bright in its part's colour when the song has the section, else grey, struck out
			Drawn* lb = place<Drawn>(this, SLB[k]);
			lb->paintLit = [=](const DrawArgs& a) {
				bool here = mod ? secHere[k] : true;
				NVGcolor c = here ? PART_COL[SEC_PART[k]] : nvgRGB(0x55, 0x58, 0x62);
				text(a.vg, bold(), lb->box.size.x / 2, lb->box.size.y / 2 + 0.5f, SEC_IN_NAMES[k], 5.4f, c, NVG_ALIGN_CENTER, 0.3f);
				if (!here) {
					float w = std::min(lb->box.size.x - 2, 6.f + 3.4f * (float) std::strlen(SEC_IN_NAMES[k]));
					nvgBeginPath(a.vg);
					nvgMoveTo(a.vg, lb->box.size.x / 2 - w / 2, lb->box.size.y / 2 + 0.3f);
					nvgLineTo(a.vg, lb->box.size.x / 2 + w / 2, lb->box.size.y / 2 + 0.3f);
					nvgStrokeColor(a.vg, c);
					nvgStrokeWidth(a.vg, 0.8f);
					nvgStroke(a.vg);
				}
			};
		}
		addOutput(createOutputCentered<PJ301MPort>(mm(L::OUT_PHRASE), module, TechnoMod::PHRASE_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm(L::OUT_PART), module, TechnoMod::PART_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm(L::OUT_BAR), module, TechnoMod::BAR_OUTPUT));
		TField* pr = place<TField>(this, L::F_PRESET);
		if (module)
			pr->mapSelect(module, TechnoMod::PRESETSEL_PARAM, &module->jsonCode);
		pr->prefix = "PRESET";
		pr->value = [=]() { return std::string(PRESETS[mod ? mod->preset : 0].name); };
		pr->count = []() { return 6; };
		pr->current = [=]() { return mod ? mod->preset : 0; };
		pr->item = [](int i) { return std::string(PRESETS[i].name); };
		pr->choose = [=](int i) {
			if (mod)
				mod->loadPreset(i);
		};
		TButton* gr = place<TButton>(this, L::BTN_GROW);
		gr->label = "GROW";
		gr->lit = [=]() { return mod && mod->grow; };
		gr->colOf = [=]() { return mod && mod->grow ? dark(RED, 0.6f) : BTN; };
		gr->act = [=]() {
			if (mod)
				mod->grow = !mod->grow;
		};
		TButton* ch = place<TButton>(this, L::BTN_CHORD);
		ch->label = "CHORD";
		ch->lit = [=]() { return mod && mod->chord; };
		ch->colOf = [=]() { return mod && mod->chord ? dark(RED, 0.6f) : BTN; };
		ch->act = [=]() {
			if (mod)
				mod->chord = !mod->chord;
		};
		TButton* fr = place<TButton>(this, L::BTN_FREEZE);
		fr->label = "FREEZE";
		fr->lit = [=]() { return mod && mod->freeze; };
		fr->act = [=]() {
			if (mod)
				mod->freeze = !mod->freeze;
		};
		mv = place<MatrixView>(this, L::A_MATRIX);
		mv->mod = module;
		mv->gm = [=]() { return gm(); };
		Drawn* d = place<Drawn>(this, L::DISP_MOD);
		d->paintLit = [=](const DrawArgs& a) { drawDisplay(a.vg, d->box.size); };
		Drawn* ln = place<Drawn>(this, L::A_LINK);
		ln->paintLit = [=](const DrawArgs& a) {
			bool on = gm() != nullptr;
			nvgBeginPath(a.vg);
			nvgCircle(a.vg, ln->box.size.x / 2, ln->box.size.y / 2 - 3, 3.5f);
			nvgFillColor(a.vg, on ? (mod && mod->link.fellBack ? nvgRGB(0xff, 0xa0, 0x30) : GREEN) : nvgRGB(0x44, 0x46, 0x50));
			nvgFill(a.vg);
			text(a.vg, bold(), ln->box.size.x / 2, ln->box.size.y - 3, "LINK", 6.5f, DIM, NVG_ALIGN_CENTER);
		};
	}

	void drawDisplay(NVGcontext* vg, Vec sz) {
		using namespace tui;
		auto f = bold();
		GenraTechno* g = gm();
		if (!mod)
			return;
		std::vector<Section> secs;
		bool fromAblauf = false;
		if (g) {
			std::lock_guard<std::mutex> lk(g->viewMx);
			secs = g->view.sections;
			fromAblauf = g->view.fromAblauf;
		}
		int part = mod->pubPart.load();
		bool run = g ? g->running.load() : !mod->pubOwn.load();
		// head
		std::string name = PART_NAMES[part % 6];
		if (g && part == P_DROP)
			name += string::f(" %d", g->pubDropIdx.load() + 1);
		text(vg, f, 8, 12, name, 13.f, PART_COL[part % 6], NVG_ALIGN_LEFT, 1.f);
		if (g) {
			text(vg, f, 92, 12,
			     string::f("BAR %d / %d  ·  %.0f BPM  ·  %s %s", g->pubSecBar.load() + 1, g->pubSecBars.load(), g->bpm(),
			               KEY_NAMES[g->key], SCALES[g->effectiveScale()].name),
			     7.5f, DIM, NVG_ALIGN_LEFT, 0.5f);
			int np = g->pubNextPart.load();
			std::string nx = std::string("NEXT: ") + PART_NAMES[np % 6];
			// the next build's type, if one comes
			int curIdx = g->pubSecIdx.load();
			for (size_t i = curIdx + 1; i < secs.size(); i++)
				if (secs[i].part == P_BUILD) {
					nx += std::string(" · BUILD: ") + BUILD_NAMES[secs[i].build % NUM_BUILDS] + string::f(" %d", (int) secs[i].bars);
					break;
				}
			text(vg, f, sz.x - 8, 12, nx, 7.5f, YEL, NVG_ALIGN_RIGHT, 0.5f);
		}
		else
			text(vg, f, sz.x - 8, 12, "NO GENRA TECHNO: CLK AND PART INPUTS", 7.5f, DIM, NVG_ALIGN_RIGHT, 0.5f);
		// the song strip and one row per lane over the whole song
		if (secs.empty()) {
			Section s;
			s.part = (uint8_t) part;
			s.bars = 8;
			secs.push_back(s);
		}
		float x0 = 26, x1 = sz.x - 8, y0 = 21, sh = 10;
		int total = std::max(1, songBars(secs));
		float x = x0;
		int curIdx = g ? g->pubSecIdx.load() : 0;
		std::vector<std::pair<float, float>> spans;
		for (size_t i = 0; i < secs.size(); i++) {
			float w = (x1 - x0) * secs[i].bars / total;
			bool now = (int) i == curIdx && run;
			NVGcolor c = PART_COL[secs[i].part % 6];
			roundRect(vg, x + 0.4f, y0, std::max(1.f, w - 0.8f), sh, 1.f, now ? c : dark(c, 0.45f));
			if (w > 24)
				textFit(vg, f, x + w / 2, y0 + sh / 2 + 0.5f, PART_NAMES[secs[i].part % 6], 6.f, w - 3, now ? BG : TXT, NVG_ALIGN_CENTER, 0.4f);
			spans.push_back(std::make_pair(x, w));
			x += w;
		}
		float lh = (sz.y - y0 - sh - 22) / 4;
		for (int j = 0; j < 4; j++) {
			float ly = y0 + sh + 3 + j * lh;
			text(vg, f, 10, ly + lh / 2, std::string(1, (char) ('A' + j)), 8.f, LANE_COL[j], NVG_ALIGN_LEFT);
			roundRect(vg, x0, ly + 1, x1 - x0, lh - 2, 1.f, nvgRGB(0x0d, 0x0e, 0x12));
			for (size_t i = 0; i < secs.size(); i++) {
				int n = mod->effN.load();
				int s = (int) i < n ? mod->effShape[i][j] : mod->shape[secs[i].part % 6][j];
				float sx = spans[i].first + 1.5f, sw = std::max(2.f, spans[i].second - 3.f);
				MatrixView::drawShape(vg, sx, ly + 2.5f, sw, lh - 5, s, s == SH_OFF ? nvgRGB(0x44, 0x48, 0x52) : LANE_COL[j], j);
				// from outside: what came in while the section played, dashed
				if (isIn(s) && i < hist.size()) {
					bool any = false;
					nvgBeginPath(vg);
					for (int q = 0; q < 64; q++) {
						float v = hist[i][j * 64 + q];
						if (v < 0.f)
							continue;
						float px = sx + sw * q / 63.f, py = ly + 2.5f + (lh - 5) * (1.f - v);
						if (!any)
							nvgMoveTo(vg, px, py);
						else
							nvgLineTo(vg, px, py);
						any = true;
					}
					if (any) {
						nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 150));
						nvgStrokeWidth(vg, 1.0f);
						nvgStroke(vg);
					}
				}
			}
		}
		if (g && run) {
			float pos = (g->pubSongBar.load() + g->pubStepInBar.load() / 16.f) / total;
			float px = x0 + (x1 - x0) * std::min(1.f, pos);
			roundRect(vg, px - 0.8f, y0 - 2, 1.6f, sh + 4 * lh + 5, 0.5f, WHITE);
		}
		// bottom
		std::string bl = g ? (fromAblauf ? "SONG: ABLAUF" : "SONG: LIVE") : "SONG: OWN CLOCK";
		if (g) {
			int drops = 0;
			for (const Section& s : secs)
				drops += s.part == P_DROP;
			if (part == P_DROP)
				bl += string::f("  ·  DROP %d OF %d", g->pubDropIdx.load() + 1, std::max(1, drops));
		}
		bl += mod->grow ? "  ·  GROW ON" : "";
		bl += mod->freeze ? "  ·  FROZEN" : "";
		text(vg, f, sz.x - 8, sz.y - 7, bl, 7.f, DIM, NVG_ALIGN_RIGHT, 0.5f);
		std::string tg_;
		for (int j = 0; j < 4; j++)
			tg_ += std::string(j ? "   " : "") + (char) ('A' + j) + " " + TARGETS[mod->target[j]];
		text(vg, f, 8, sz.y - 7, tg_, 7.f, DIM, NVG_ALIGN_LEFT, 0.4f);
	}

	void step() override {
		ModuleWidget::step();
		if (!btnsMapped) { // the buttons for MIDI-MAP (the panel is built by now)
			tui::mapButtons(this, module, TechnoMod::BTN_PARAM, BTNS, NUM_BTNS);
			if (mod)
				tui::mapFields(this, module, TechnoMod::FLD_PARAM, FLDS, 8, &mod->jsonCode);
			btnsMapped = true;
		}
		if (!mod)
			return;
		mod->link.uiStep(APP->window->getLastFrameDuration());
		GenraTechno* g = gm();
		Cols c = columnsOf(g);
		// the table the audio thread reads: every section's movements (its own or its part's) and its jack
		int n = c.song ? std::min((int) c.secs.size(), MAX_SECS) : 0;
		for (int i = 0; i < n; i++) {
			int part = c.secs[i].part % NUM_PARTS;
			const TechnoMod::SecOv& o = mod->ov[i];
			for (int j = 0; j < 4; j++)
				mod->effShape[i][j] = (int8_t) (o.part == part && o.shape[j] >= 0 ? o.shape[j] : mod->shape[part][j]);
			mod->effPart[i] = (int8_t) part;
			mod->secJack[i] = (int8_t) c.jack[i];
		}
		mod->effN = n;
		// which section jacks the song has, and their tooltips
		uint32_t sig = (uint32_t) n * 2654435761u;
		for (int k = 0; k < NUM_SEC_IN; k++) {
			secHere[k] = false;
			secTip[k] = std::string(SEC_IN_NAMES[k]) + (c.song ? " (not in this song)" : "");
		}
		for (int i = 0; i < (int) c.secs.size(); i++) {
			sig = sig * 31u + c.secs[i].part * 7u + c.secs[i].bars;
			if (!c.song)
				continue;
			int k = c.jack[i];
			if (!secHere[k])
				secTip[k] = std::string(SEC_IN_NAMES[k]) + string::f(" · row %d · %d bars", i + 1, (int) c.secs[i].bars);
			else
				secTip[k] += string::f(", row %d", i + 1);
			secHere[k] = true;
		}
		if (!c.song)
			for (int k = 0; k < NUM_SEC_IN; k++)
				secHere[k] = true;
		for (int k = 0; k < NUM_SEC_IN; k++)
			if (engine::PortInfo* pi = mod->inputInfos[TechnoMod::SEC_INPUT + k])
				pi->name = secTip[k] + " from outside (IN SEC)";
		// what comes in from outside, recorded over the section that plays (a new song: forget it)
		if (sig != histSig || hist.size() != (size_t) n) {
			histSig = sig;
			hist.assign(n, std::array<float, 4 * 64>());
			for (auto& h : hist)
				h.fill(-1.f);
		}
		int cur = mod->pubSec.load();
		if (g && g->running.load() && cur >= 0 && cur < n) {
			float t = clamp(g->pubSecPos.load(), 0.f, 1.f);
			int q = clamp((int) (t * 63.f + 0.5f), 0, 63);
			for (int j = 0; j < 4; j++) {
				float v = mod->extNow[j].load();
				if (v >= 0.f)
					hist[cur][j * 64 + q] = v;
			}
		}
	}
	void appendContextMenu(Menu* menu) override {
		if (mod)
			mod->link.appendMenu(menu);
	}
};

Model* modelTechnoMod = createModel<TechnoMod, TechnoModWidget>("TechnoMod");
