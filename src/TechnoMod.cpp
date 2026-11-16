// SPDX-License-Identifier: GPL-3.0-only
// GENRA TECHNO MOD: four CV lanes that move over the song. For every part of the song (INTRO … OUTRO) and every lane
// a movement (low, mid, high, rise, fall, swell, fade, wave, pump, wander, steps, jump, dip, off); DEPTH, SMOOTH and
// BARS per lane, TARGET only names it. It follows the song GENRA TECHNO plays: ABLAUF's when ABLAUF is connected, else
// GENRA TECHNO's own LIVE song; without a GENRA TECHNO it counts its CLK input and takes the part from PART (0..5 V).
// GROW makes later drops bigger, CHORD adds a little lift on every chord change, FREEZE holds the outputs.
#include "TechnoModule.hpp"
#include "TechnoUI.hpp"
#include "TechnoModLayout.hpp"
#include <algorithm>
#include <cmath>

using namespace tg;
namespace L = technomod_layout;

enum Shape { SH_OFF, SH_LOW, SH_MID, SH_HIGH, SH_RISE, SH_FALL, SH_SWELL, SH_FADE, SH_WAVE, SH_PUMP, SH_WANDER, SH_STEPS, SH_JUMP, SH_DIP, NUM_SHAPES };
static const char* const SHAPE_NAMES[NUM_SHAPES] = {"OFF",  "LOW",  "MID",    "HIGH",  "RISE", "FALL", "SWELL",
                                                    "FADE", "WAVE", "PUMP",   "WANDER", "STEPS", "JUMP", "DIP"};
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
	enum ParamId { ENUMS(DEPTH_PARAM, 4), ENUMS(SMOOTH_PARAM, 4), NUM_PARAMS };
	enum InputId { CLK_INPUT, RST_INPUT, PART_INPUT, DEPTH_INPUT, NUM_INPUTS };
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

	TechnoMod() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, 0);
		for (int j = 0; j < 4; j++) {
			std::string n(1, (char) ('A' + j));
			configParam(DEPTH_PARAM + j, 0.f, 1.f, 0.8f, "Lane " + n + " depth", " %", 0.f, 100.f);
			configParam(SMOOTH_PARAM + j, 0.f, 1.f, 0.3f, "Lane " + n + " smooth (glide between the parts)", " %", 0.f, 100.f);
			configOutput(LANE_OUTPUT + j, "Lane " + n + " (UNI 0..10 V, BI ±5 V)");
		}
		configInput(CLK_INPUT, "Clock (16ths; only without a GENRA TECHNO)");
		configInput(RST_INPUT, "Reset (only without a GENRA TECHNO)");
		configInput(PART_INPUT, "Part (0 V intro … 5 V outro; only without a GENRA TECHNO)");
		configInput(DEPTH_INPUT, "Depth CV (±10 V: ±100 % on all lanes)");
		configOutput(PHRASE_OUTPUT, "Phrase (a trigger every 8 bars)");
		configOutput(PART_OUTPUT, "Part (0 V intro … 5 V outro)");
		configOutput(BAR_OUTPUT, "Bar (trigger)");
		link.owner = this;
		loadPreset(0);
	}
	void loadPreset(int p) {
		preset = clamp(p, 0, 5);
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

	float laneValue(int j, const Pos& p) {
		int s = shape[p.part % 6][j];
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
			float dcv = clamp(inputs[DEPTH_INPUT].getVoltage() / 10.f, -1.f, 1.f);
			float growMul = (grow && p.part == P_DROP) ? 1.f + 0.25f * std::min(2, p.dropIdx) : 1.f;
			for (int j = 0; j < 4; j++) {
				bump[j] *= std::exp(-dt / 0.12f);
				if (freeze || !p.running)
					continue;
				int s = shape[p.part % 6][j];
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
		return root;
	}
	void dataFromJson(json_t* root) override {
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
	}
};

// ================================================================ the panel
struct MatrixView : widget::OpaqueWidget {
	TechnoMod* mod = nullptr;
	float labW = 0, cw = 0, ch = 0, gap = 0;
	void geo() {
		gap = mm2px(1.1f);
		labW = mm2px(13.5f);
		cw = (box.size.x - mm2px(15.f) - 3 * gap) / 4;
		ch = (box.size.y - 5 * gap) / 6;
	}
	void draw(const DrawArgs& args) override {
		using namespace tui;
		geo();
		NVGcontext* vg = args.vg;
		auto f = bold();
		int cur = mod ? mod->pubPart.load() : -1;
		float x0 = mm2px(15.f);
		for (int i = 0; i < 6; i++) {
			float y = i * (ch + gap);
			bool now = i == cur;
			roundRect(vg, 0, y, labW, ch, 2.f, now ? dark(PART_COL[i], 0.4f) : nvgRGB(0x19, 0x1a, 0x1f));
			text(vg, f, labW / 2, y + ch / 2 + 0.5f, PART_NAMES[i], 7.f, PART_COL[i], NVG_ALIGN_CENTER, 0.6f);
			for (int j = 0; j < 4; j++) {
				float x = x0 + j * (cw + gap);
				NVGcolor c = LANE_COL[j];
				roundRect(vg, x, y, cw, ch, 2.f, now ? dark(c, 0.35f) : BTN);
				if (now)
					frame(vg, x + 0.5f, y + 0.5f, cw - 1, ch - 1, 2.f, c, 1.f);
				int s = mod ? mod->shape[i][j] : 0;
				drawShape(vg, x + 4, y + 3, cw - 8, ch - 6, s, s == SH_OFF ? nvgRGB(0x44, 0x48, 0x52) : c, j);
			}
		}
	}
	static void drawShape(NVGcontext* vg, float x, float y, float w, float h, int s, NVGcolor c, int lane) {
		nvgBeginPath(vg);
		if (s == SH_OFF) {
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
		geo();
		float x0 = mm2px(15.f);
		if (p.x < x0)
			return false;
		*j = (int) ((p.x - x0) / (cw + gap));
		*i = (int) (p.y / (ch + gap));
		return *i >= 0 && *i < 6 && *j >= 0 && *j < 4;
	}
	void onButton(const ButtonEvent& e) override {
		if (!mod || e.action != GLFW_PRESS)
			return;
		int i, j;
		if (!cellAt(e.pos, &i, &j))
			return;
		e.consume(this);
		if (e.button == GLFW_MOUSE_BUTTON_LEFT)
			mod->shape[i][j] = (mod->shape[i][j] + 1) % NUM_SHAPES;
		else if (e.button == GLFW_MOUSE_BUTTON_RIGHT) {
			ui::Menu* menu = createMenu();
			menu->addChild(createMenuLabel(string::f("%s · lane %c", PART_NAMES[i], 'A' + j)));
			TechnoMod* m = mod;
			for (int s = 0; s < NUM_SHAPES; s++)
				menu->addChild(createCheckMenuItem(SHAPE_NAMES[s], "", [=]() { return m->shape[i][j] == s; },
				                                   [=]() { m->shape[i][j] = s; }));
		}
	}
	void onHoverScroll(const HoverScrollEvent& e) override {
		int i, j;
		if (!mod || !cellAt(e.pos, &i, &j))
			return;
		int d = e.scrollDelta.y > 0 ? -1 : (e.scrollDelta.y < 0 ? 1 : 0);
		mod->shape[i][j] = clamp(mod->shape[i][j] + d, 0, NUM_SHAPES - 1);
		e.consume(this);
	}
};

struct TechnoModWidget : ModuleWidget {
	TechnoMod* mod = nullptr;
	GenraTechno* gm() { return mod ? mod->link.main.load() : nullptr; }

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
		addOutput(createOutputCentered<PJ301MPort>(mm(L::OUT_PHRASE), module, TechnoMod::PHRASE_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm(L::OUT_PART), module, TechnoMod::PART_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm(L::OUT_BAR), module, TechnoMod::BAR_OUTPUT));
		TField* pr = place<TField>(this, L::F_PRESET);
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
		MatrixView* mv = place<MatrixView>(this, L::A_MATRIX);
		mv->mod = module;
		Drawn* d = place<Drawn>(this, L::DISP_MOD);
		d->paintLit = [=](const DrawArgs& a) { drawDisplay(a.vg, d->box.size); };
		Drawn* ln = place<Drawn>(this, L::A_LINK);
		ln->paintLit = [=](const DrawArgs& a) {
			bool on = gm() != nullptr;
			nvgBeginPath(a.vg);
			nvgCircle(a.vg, ln->box.size.x / 2, ln->box.size.y / 2 - 3, 3.5f);
			nvgFillColor(a.vg, on ? GREEN : nvgRGB(0x44, 0x46, 0x50));
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
			               KEY_NAMES[g->key], SCALES[styles()[g->style].mode].name),
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
				int s = mod->shape[secs[i].part % 6][j];
				MatrixView::drawShape(vg, spans[i].first + 1.5f, ly + 2.5f, std::max(2.f, spans[i].second - 3.f), lh - 5,
				                      s, s == SH_OFF ? nvgRGB(0x44, 0x48, 0x52) : LANE_COL[j], j);
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
		if (mod)
			mod->link.uiStep(APP->window->getLastFrameDuration());
	}
	void appendContextMenu(Menu* menu) override {
		if (mod)
			mod->link.appendMenu(menu);
	}
};

Model* modelTechnoMod = createModel<TechnoMod, TechnoModWidget>("TechnoMod");
