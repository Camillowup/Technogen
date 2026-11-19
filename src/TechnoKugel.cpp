// SPDX-License-Identifier: GPL-3.0-only
// GENRA TECHNO KUGEL: an XY pad with a ball that follows physics (after the Kaoss Physics of the Korg modwave).
// Throw the ball with the mouse; GRAVITY pulls it in the direction of TILT (endless, 360°), FRICTION slows it, MASS
// makes kicks and pushes weaker, BOUNCE is how much of its speed it keeps at a wall. WALL: BOUNCE, WRAP, STICK, SLIDE,
// ROUND (a round arena). KICK (or a GATE, VEL = strength) throws it in a random direction, HOLD holds it.
// REC records the ball's way in time: it starts on the next bar and runs LOOP LEN bars (1, 2, 4, 8), then LOOP plays
// it back in sync (RESET starts it again at bar 1). The time comes from the CLK jack (16ths, 8ths, quarters or
// 24 PPQN: right-click), else from the GENRA it follows (right-click › Follow GENRA: GENRA, GENRA TECHNO, GENRA CHOP,
// through the link hub), else from its own clock (120 BPM).
// Outputs X, Y and SPEED in the range of RANGE (−10..0 V, ±5 V, 0..10 V), SMOOTH glides them; HIT: a trigger at every
// wall.
#include "plugin.hpp"
#include "TechnoUI.hpp"
#include "TechnoKugelLayout.hpp"
#include "link/Follow.hpp"
#include "link/GenraPorts.hpp"
#include <atomic>
#include <climits>
#include <cmath>
#include <vector>

namespace L = technokugel_layout;

// follow::Ref is only declared in the copy of Follow.hpp; GENRA TECHNO has no Follow.cpp, so KUGEL brings what it uses
// of it: the reference unregisters itself at the hub when it goes.
follow::Ref::~Ref() { genralink::hub().unref(this); }

namespace {

const NVGcolor AC = nvgRGB(0x5e, 0xc8, 0xe6); // KUGEL's accent (light blue)

// the buttons MIDI-MAP can press (BTN_PARAM + i; only ever append)
const tui::BtnDef BTNS[] = {{L::BTN_KICK, "KICK"}, {L::BTN_HOLD, "HOLD"}, {L::BTN_REC, "REC"}, {L::BTN_LOOP, "LOOP"}};
const int NUM_BTNS = 4;
// the fields MIDI-MAP can turn through (FLD_PARAM + i; only ever append)
const tui::BtnDef FLDS[] = {{L::F_WALL, "WALL"}, {L::F_RANGE, "RANGE"}, {L::F_LOOP, "LOOP LEN"}};
const int NUM_FLDS = 3;

enum Wall { W_BOUNCE, W_WRAP, W_STICK, W_SLIDE, W_ROUND, NUM_WALLS };
const char* const WALL_NAMES[NUM_WALLS] = {"BOUNCE", "WRAP", "STICK", "SLIDE", "ROUND"};
const char* const WALL_TIPS[NUM_WALLS] = {"BOUNCE (it bounces off)", "WRAP (out on one side, in on the other)",
                                          "STICK (it stays at the wall until a kick)", "SLIDE (it slides along the wall)",
                                          "ROUND (a round arena)"};
const char* const RANGE_NAMES[3] = {"-10..0 V", "\xc2\xb1" "5 V", "0..10 V"};
const int LOOP_BARS[4] = {1, 2, 4, 8};
const char* const LOOP_NAMES[4] = {"1 BAR", "2 BARS", "4 BARS", "8 BARS"};
// the CLK jack: 16ths per pulse
const float CLK_RES[4] = {1.f, 2.f, 4.f, 1.f / 6.f};
const char* const CLK_RES_NAMES[4] = {"16ths (as GENRA's clock)", "8ths", "Quarters", "24 PPQN"};
// what kicks the ball from the GENRA it follows (the track's gate, velocity: strength)
enum KickFrom { K_DRUMS, K_BASS, K_MEL, K_PAD, K_STAB, K_NONE, NUM_KICKS };
const char* const KICK_NAMES[NUM_KICKS] = {"Drums (kick)", "Bass", "Melody / lead", "Pads / atmo", "Stab (GENRA TECHNO)",
                                           "Nothing (only the clock)"};

struct Preset {
	const char* name;
	float grav, tilt, fric, mass, bnc, force, smooth;
	int wall;
};
// knob values: GRAVITY, TILT (°), FRICTION, MASS, BOUNCE, FORCE, SMOOTH (0..1 except TILT)
const Preset PRESETS[] = {
    {"PINBALL", 0.31f, 20.f, 0.18f, 0.5f, 0.74f, 0.6f, 0.1f, W_BOUNCE},
    {"SUPERBALL", 0.45f, 0.f, 0.05f, 0.4f, 0.95f, 0.7f, 0.05f, W_BOUNCE},
    {"FEATHER", 0.08f, 0.f, 0.55f, 0.1f, 0.4f, 0.4f, 0.3f, W_BOUNCE},
    {"MOON", 0.1f, 0.f, 0.02f, 0.5f, 0.8f, 0.5f, 0.15f, W_BOUNCE},
    {"BILLIARD", 0.f, 0.f, 0.22f, 0.6f, 0.9f, 0.7f, 0.05f, W_BOUNCE},
    {"PING PONG", 0.f, 0.f, 0.f, 0.5f, 1.f, 0.6f, 0.f, W_BOUNCE},
    {"HONEY", 0.25f, 0.f, 0.8f, 0.7f, 0.2f, 0.8f, 0.4f, W_BOUNCE},
    {"BRICK", 0.6f, 0.f, 0.3f, 1.f, 0.15f, 0.9f, 0.1f, W_BOUNCE},
    {"SIDEWIND", 0.3f, 90.f, 0.1f, 0.5f, 0.7f, 0.6f, 0.1f, W_BOUNCE},
    {"RAIN", 0.35f, 0.f, 0.2f, 0.4f, 0.5f, 0.5f, 0.1f, W_WRAP},
    {"DRIFT", 0.f, 0.f, 0.06f, 0.5f, 0.6f, 0.4f, 0.35f, W_WRAP},
    {"ASTEROIDS", 0.05f, 135.f, 0.f, 0.5f, 0.6f, 0.6f, 0.f, W_WRAP},
    {"GLUE", 0.2f, 0.f, 0.2f, 0.5f, 0.5f, 0.7f, 0.2f, W_STICK},
    {"MAGNET", 0.4f, 270.f, 0.1f, 0.5f, 0.5f, 0.8f, 0.15f, W_STICK},
    {"SKATE", 0.3f, 30.f, 0.05f, 0.5f, 0.5f, 0.6f, 0.15f, W_SLIDE},
    {"GUTTER", 0.45f, 0.f, 0.1f, 0.6f, 0.4f, 0.7f, 0.1f, W_SLIDE},
    {"ORBIT", 0.f, 0.f, 0.f, 0.5f, 1.f, 0.6f, 0.1f, W_ROUND},
    {"BOWL", 0.35f, 0.f, 0.12f, 0.5f, 0.6f, 0.6f, 0.15f, W_ROUND},
    {"ROULETTE", 0.15f, 0.f, 0.15f, 0.4f, 0.85f, 0.9f, 0.05f, W_ROUND},
    {"SLOW MOTION", 0.12f, 0.f, 0.35f, 0.9f, 0.6f, 0.3f, 0.6f, W_BOUNCE},
};
const int NUM_PRESETS = (int) (sizeof(PRESETS) / sizeof(PRESETS[0]));

const int LOOP_N = 2048; // points over the loop, whatever its length

inline float wrap360(float d) {
	d = std::fmod(d, 360.f);
	return d < 0.f ? d + 360.f : d;
}

} // namespace

struct TechnoKugel : Module {
	enum ParamId {
		GRAV_PARAM,
		TILT_PARAM,
		FRIC_PARAM,
		MASS_PARAM,
		BNC_PARAM,
		FORCE_PARAM,
		SMOOTH_PARAM,
		ENUMS(BTN_PARAM, 4),
		PRESETSEL_PARAM,
		ENUMS(FLD_PARAM, 3),
		NUM_PARAMS
	};
	enum InputId { GATE_INPUT, VEL_INPUT, CLK_INPUT, RST_INPUT, GRAV_INPUT, TILT_INPUT, FRIC_INPUT, BNC_INPUT, PUSHX_INPUT,
		           PUSHY_INPUT, REC_INPUT, NUM_INPUTS };
	enum OutputId { X_OUTPUT, Y_OUTPUT, SPD_OUTPUT, HIT_OUTPUT, NUM_OUTPUTS };
	enum RecState { R_IDLE, R_ARMED, R_REC };
	enum Sync { S_FREE, S_CLK, S_FOLLOW };

	// ---- settings (UI thread writes, audio reads)
	std::atomic<int> wall{W_BOUNCE}, range{1}, loopLen{1}, clockRes{0};
	std::atomic<bool> hold{false};
	int preset = 0;
	// follow: -1 off, else automatic (wantId -1) or that module
	std::atomic<bool> followOn{true};
	int64_t wantId = -1;
	std::atomic<int> kickFrom{K_DRUMS};
	follow::Ref src;
	float uiTimer = 0.f;

	// ---- requests from the panel
	std::atomic<bool> kickReq{false}, recReq{false}, loopReq{false}, clearReq{false};
	std::atomic<bool> dragOn{false}, throwReq{false};
	std::atomic<float> dragX{0.f}, dragY{0.f}, throwVx{0.f}, throwVy{0.f};

	// ---- the ball (audio thread); screen coordinates: x right, y down, -1..1
	float px = 0.f, py = 0.f, vx = 0.f, vy = 0.f;
	bool stuck = false;
	float kickStrength = 1.f;
	bool kickPending = false;
	// outputs
	float tX = 0.f, tY = 0.f, tS = 0.f, oX = 0.f, oY = 0.f, oS = 0.f, slew = 1.f;
	dsp::PulseGenerator hitP;
	int ctl = 0;

	// ---- time
	dsp::SchmittTrigger clkTrig, rstTrig, gateTrig, recTrig;
	bool fClk = false, fRst = false, fGate = false;
	double base = -1.0;   // 16ths at the last clock pulse (-1: none since the reset)
	float since = 0.f, gap = 0.f;
	double freePos = 0.0; // the own clock
	int lastSync = -1;
	double pos16 = 0.0;
	bool running = true;
	bool resetHappened = false;

	// ---- the loop
	int recState = R_IDLE;
	bool hasLoop = false, playing = false;
	double recStart = 0.0, anchor = 0.0;
	int recLast = -1;
	std::vector<float> lx, ly;
	std::vector<uint8_t> lhit;
	int lastPlayIdx = -1;
	int64_t lastBar = INT64_MIN;

	// ---- published for the panel
	std::atomic<float> pubX{0.f}, pubY{0.f};
	std::atomic<int> pubHits{0};
	std::atomic<int> pubRec{R_IDLE}, pubSync{S_FREE};
	std::atomic<bool> pubPlaying{false}, pubHasLoop{false}, pubFollowFeeds{false};
	std::atomic<float> pubLoopPos{0.f}, pubBpm{120.f}, pubRecPos{0.f};
	std::atomic<float> eff[5]; // GRAVITY 0..1, TILT °, FRICTION, MASS (kg), BOUNCE
	uint32_t jsonCode = 0;

	TechnoKugel() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, 0);
		configParam(GRAV_PARAM, 0.f, 1.f, 0.31f, "Gravity", " g", 0.f, 2.f);
		configParam(TILT_PARAM, 0.f, 360.f, 20.f, "Tilt (where gravity pulls; endless, 0° down)", "°");
		configParam(FRIC_PARAM, 0.f, 1.f, 0.18f, "Friction", " %", 0.f, 100.f);
		configParam(MASS_PARAM, 0.f, 1.f, 0.5f, "Mass (heavier: kicks and pushes move it less)", " kg", 16.f, 0.25f);
		configParam(BNC_PARAM, 0.f, 1.f, 0.74f, "Bounce (the speed it keeps at a wall)", " %", 0.f, 100.f);
		configParam(FORCE_PARAM, 0.f, 1.f, 0.6f, "Force (of KICK and GATE)", " %", 0.f, 100.f);
		configParam(SMOOTH_PARAM, 0.f, 1.f, 0.1f, "Smooth (glide of X, Y, SPEED)", " %", 0.f, 100.f);
		tui::configButtons(this, BTN_PARAM, BTNS, NUM_BTNS);
		configParam(PRESETSEL_PARAM, 0.f, 1.f, 0.f, "Preset select (for MIDI-MAP: a knob goes through the presets)", " %", 0.f, 100.f);
		paramQuantities[PRESETSEL_PARAM]->randomizeEnabled = false;
		paramQuantities[PRESETSEL_PARAM]->resetEnabled = false;
		tui::configSelects(this, FLD_PARAM, FLDS, NUM_FLDS);
		configInput(GATE_INPUT, "Gate (kicks the ball; free: the track of the GENRA it follows)");
		configInput(VEL_INPUT, "Velocity (0..10 V: the strength of the gate's kick)");
		configInput(CLK_INPUT, "Clock (16ths; other resolutions: right-click › Clock input)");
		configInput(RST_INPUT, "Reset (the loop starts again at bar 1)");
		configInput(GRAV_INPUT, "Gravity CV (10 V: the whole range)");
		configInput(TILT_INPUT, "Tilt CV (0..10 V: 0..360°)");
		configInput(FRIC_INPUT, "Friction CV (10 V: the whole range)");
		configInput(BNC_INPUT, "Bounce CV (10 V: the whole range)");
		configInput(PUSHX_INPUT, "Push X (±5 V: pushes the ball left / right)");
		configInput(PUSHY_INPUT, "Push Y (±5 V: pushes the ball down / up)");
		configInput(REC_INPUT, "REC (a trigger: like the REC key)");
		configOutput(X_OUTPUT, "X (RANGE: -10..0 V, ±5 V or 0..10 V)");
		configOutput(Y_OUTPUT, "Y (up is high; RANGE as X)");
		configOutput(SPD_OUTPUT, "Speed of the ball (RANGE as X)");
		configOutput(HIT_OUTPUT, "Hit (a trigger at every wall)");
		lx.assign(LOOP_N, 0.f);
		ly.assign(LOOP_N, 0.f);
		lhit.assign(LOOP_N, 0);
		for (int i = 0; i < 5; i++)
			eff[i] = 0.f;
		loadPreset(0);
	}

	void loadPreset(int i) {
		preset = clamp(i, 0, NUM_PRESETS - 1);
		const Preset& p = PRESETS[preset];
		params[GRAV_PARAM].setValue(p.grav);
		params[TILT_PARAM].setValue(p.tilt);
		params[FRIC_PARAM].setValue(p.fric);
		params[MASS_PARAM].setValue(p.mass);
		params[BNC_PARAM].setValue(p.bnc);
		params[FORCE_PARAM].setValue(p.force);
		params[SMOOTH_PARAM].setValue(p.smooth);
		wall = p.wall;
	}

	void onRemove(const RemoveEvent& e) override { genralink::hub().gone(this); }

	/** UI thread: find the GENRA to follow (again). */
	void uiStep(float dt) {
		uiTimer -= dt;
		if (uiTimer > 0.f)
			return;
		uiTimer = 0.5f;
		Module* want = nullptr;
		genralink::GenraInfo wi;
		if (followOn.load())
			for (int64_t id : APP->engine->getModuleIds()) {
				if (wantId >= 0 && id != wantId)
					continue;
				Module* m = APP->engine->getModule(id);
				genralink::GenraInfo gi;
				if (m && m != this && genralink::hub().genraInfo(m, &gi) && gi.followPub) {
					want = m;
					wi = gi;
					break;
				}
			}
		if (want == src.to.load())
			return;
		genralink::hub().point(&src, this, nullptr);
		src.genra = wi;
		if (want)
			genralink::hub().point(&src, this, want);
	}
	/** UI thread: the GENRAs there are (id, name). */
	static std::vector<std::pair<int64_t, std::string>> sources(Module* self) {
		std::vector<std::pair<int64_t, std::string>> v;
		for (int64_t id : APP->engine->getModuleIds()) {
			Module* m = APP->engine->getModule(id);
			genralink::GenraInfo gi;
			if (m && m != self && genralink::hub().genraInfo(m, &gi))
				v.push_back(std::make_pair(id, m->model ? m->model->name : std::string("GENRA")));
		}
		return v;
	}

	float outV(float u) const { // u 0..1 in the RANGE
		switch (range.load()) {
		case 0: return u * 10.f - 10.f;
		case 2: return u * 10.f;
		default: return u * 10.f - 5.f;
		}
	}

	void hit() {
		hitP.trigger(5e-3f);
		pubHits.fetch_add(1);
	}

	/** One step of the ball. */
	void physics(float dt, float g, float tiltDeg, float fric, float mass, float bnc, float pushX, float pushY, bool& hitNow) {
		if (dragOn.load()) {
			float nx = clamp(dragX.load(), -1.f, 1.f), ny = clamp(dragY.load(), -1.f, 1.f);
			vx = 0.f, vy = 0.f;
			px = nx, py = ny;
			stuck = false;
			kickPending = false;
			return;
		}
		if (throwReq.exchange(false)) {
			vx = throwVx.load();
			vy = throwVy.load();
			stuck = false;
		}
		if (hold.load()) {
			vx = vy = 0.f;
			kickPending = false;
			return;
		}
		if (kickPending) {
			kickPending = false;
			float a = random::uniform() * 2.f * (float) M_PI;
			float sp = (1.5f + 8.f * params[FORCE_PARAM].getValue()) * kickStrength / std::sqrt(mass);
			vx += std::cos(a) * sp;
			vy += std::sin(a) * sp;
			stuck = false;
		}
		if (stuck)
			return;
		float th = tiltDeg * (float) M_PI / 180.f;
		float acc = g * 2.f * 6.f; // g 0..1 → 0..2 g, 1 g = 6 units/s²
		float ax = -std::sin(th) * acc + pushX * 2.f / mass, ay = std::cos(th) * acc + pushY * 2.f / mass;
		vx += ax * dt;
		vy += ay * dt;
		float drag = fric * fric * 8.f / std::sqrt(mass);
		float k = std::exp(-drag * dt);
		vx *= k, vy *= k;
		px += vx * dt;
		py += vy * dt;
		float e = 0.98f * bnc;
		const float THR = 0.15f;
		int w = wall.load();
		if (w == W_ROUND) {
			float r = std::sqrt(px * px + py * py);
			if (r > 1.f) {
				float nx = px / r, ny = py / r;
				px = nx * 0.9999f, py = ny * 0.9999f;
				float vn = vx * nx + vy * ny;
				if (vn > 0.f) {
					vx -= (1.f + e) * vn * nx;
					vy -= (1.f + e) * vn * ny;
					if (vn * e < THR) { // too slow to come off: it rolls along the edge
						float vn2 = vx * nx + vy * ny;
						vx -= vn2 * nx, vy -= vn2 * ny;
					}
					if (vn > THR)
						hitNow = true;
				}
			}
			return;
		}
		auto edge = [&](float& p, float& v, float& vt) {
			if (p <= 1.f && p >= -1.f)
				return;
			float s = p > 1.f ? 1.f : -1.f;
			float vn = v * s > 0.f ? std::fabs(v) : 0.f; // only what goes into the wall
			if (w == W_WRAP) {
				p -= 2.f * s;
				hitNow = true;
				return;
			}
			p = s;
			if (w == W_STICK) {
				v = 0.f, vt = 0.f;
				stuck = true;
				hitNow = true;
				return;
			}
			if (w == W_SLIDE) {
				v = 0.f;
				vt *= std::exp(-(1.f + fric * 10.f) * dt);
			}
			else if (vn > 0.f) {
				v = -e * v;
				if (vn * e < THR)
					v = 0.f; // it rests on the wall
			}
			if (vn > THR)
				hitNow = true;
		};
		edge(px, vx, vy);
		edge(py, vy, vx);
	}

	void process(const ProcessArgs& args) override {
		// ---- the GENRA it follows (its outputs as of the last complete frame)
		Module* sm = src.to.load();
		const genralink::GenraInfo gi = src.genra;
		bool fol = sm && gi.followPub && gi.outputs > genraports::RESET_OUTPUT;
		int buf = (int) ((args.frame + 1) & 1);
		auto rd = [&](int out, int ch) -> float {
			if (!fol || out >= gi.outputs)
				return 0.f;
			return gi.followPub[((size_t) buf * gi.outputs + out) * genraports::PUB_CHANNELS + ch];
		};
		// ---- time: CLK jack, else the GENRA, else the own clock
		int sync = inputs[CLK_INPUT].isConnected() ? S_CLK : (fol ? S_FOLLOW : S_FREE);
		if (sync != lastSync) {
			base = -1.0;
			gap = 0.f;
			lastSync = sync;
		}
		since += 1.f;
		bool pulse = false, rst = false;
		float per16 = 1.f;
		if (sync == S_CLK) {
			pulse = clkTrig.process(inputs[CLK_INPUT].getVoltage(), 0.1f, 1.f);
			per16 = CLK_RES[clamp(clockRes.load(), 0, 3)];
		}
		else if (sync == S_FOLLOW) {
			bool c = rd(genraports::CLOCK_OUTPUT, 0) > 1.f;
			pulse = c && !fClk;
			fClk = c;
			// the resolution of the GENRA's clock (it can be set there): from its tempo
			float sp = gi.stepPeriod ? *gi.stepPeriod : 0.f;
			if (gap > 10.f && sp > 1.f) {
				float r = gap / sp;
				per16 = r < 0.5f ? 1.f / 6.f : (r < 1.5f ? 1.f : (r < 3.f ? 2.f : 4.f));
			}
			bool rr = rd(genraports::RESET_OUTPUT, 0) > 1.f;
			rst = rr && !fRst;
			fRst = rr;
		}
		if (inputs[RST_INPUT].isConnected() && rstTrig.process(inputs[RST_INPUT].getVoltage(), 0.1f, 1.f))
			rst = true;
		if (rst) {
			base = -1.0;
			freePos = 0.0;
			anchor = 0.0;
			lastBar = INT64_MIN;
			resetHappened = true;
		}
		if (pulse) {
			if (base >= 0.0 && since > 2.f)
				gap = gap > 0.f ? 0.6f * gap + 0.4f * since : since;
			base = base < 0.0 ? 0.0 : base + per16;
			since = 0.f;
		}
		if (sync == S_FREE) {
			freePos += 8.0 * args.sampleTime; // 120 BPM: 8 16ths a second
			pos16 = freePos;
			running = true;
		}
		else {
			float g = gap > 0.f ? gap : 5000.f;
			pos16 = base < 0.0 ? 0.0 : base + std::min(0.999f, since / g) * per16;
			running = base >= 0.0 && since < 2.5f * g * std::max(1.f, per16);
		}
		// ---- the gate (a cable, else the track of the GENRA it follows)
		float gateV = 0.f, velV = 10.f;
		if (inputs[GATE_INPUT].isConnected())
			gateV = inputs[GATE_INPUT].getVoltage();
		else if (fol) {
			int k = kickFrom.load();
			int go = -1, vo = -1;
			switch (k) {
			case K_DRUMS: go = vo = genraports::DRUM_OUTPUT; break;
			case K_BASS: go = genraports::BASS_GATE_OUTPUT, vo = genraports::BASS_VEL_OUTPUT; break;
			case K_MEL: go = genraports::MEL_GATE_OUTPUT, vo = genraports::MEL_VEL_OUTPUT; break;
			case K_PAD: go = genraports::PAD_GATE_OUTPUT, vo = genraports::PAD_VEL_OUTPUT; break;
			case K_STAB: go = genraports::STAB_GATE_OUTPUT, vo = genraports::STAB_VEL_OUTPUT; break;
			default: break;
			}
			if (go >= 0) {
				for (int c = 0; c < genraports::PUB_CHANNELS; c++)
					gateV = std::max(gateV, rd(go, c));
				velV = 0.f;
				for (int c = 0; c < genraports::PUB_CHANNELS; c++)
					velV = std::max(velV, rd(vo, c));
				if (velV <= 0.f)
					velV = 10.f;
			}
		}
		if (inputs[VEL_INPUT].isConnected())
			velV = inputs[VEL_INPUT].getVoltage();
		if (gateTrig.process(gateV, 0.1f, 1.f)) {
			kickPending = true;
			kickStrength = clamp(velV / 10.f, 0.f, 1.f);
		}
		if (kickReq.exchange(false)) {
			kickPending = true;
			kickStrength = 1.f;
		}
		if (recTrig.process(inputs[REC_INPUT].getVoltage(), 0.1f, 1.f))
			recReq = true;
		pubFollowFeeds = fol;

		// ---- the ball and the loop, every 8 samples
		if (++ctl >= 8) {
			ctl = 0;
			float dt = 8.f * args.sampleTime;
			float grav = clamp(params[GRAV_PARAM].getValue() + inputs[GRAV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
			float tilt = wrap360(params[TILT_PARAM].getValue() + inputs[TILT_INPUT].getVoltage() * 36.f);
			float fric = clamp(params[FRIC_PARAM].getValue() + inputs[FRIC_INPUT].getVoltage() / 10.f, 0.f, 1.f);
			float mass = 0.25f * std::pow(16.f, params[MASS_PARAM].getValue());
			float bnc = clamp(params[BNC_PARAM].getValue() + inputs[BNC_INPUT].getVoltage() / 10.f, 0.f, 1.f);
			float pushX = clamp(inputs[PUSHX_INPUT].getVoltage(), -10.f, 10.f);
			float pushY = -clamp(inputs[PUSHY_INPUT].getVoltage(), -10.f, 10.f); // up is positive at the jack
			eff[0] = grav, eff[1] = tilt, eff[2] = fric, eff[3] = mass, eff[4] = bnc;

			int L16 = LOOP_BARS[clamp(loopLen.load(), 0, 3)] * 16;
			int64_t bar = (int64_t) std::floor(pos16 / 16.0);
			bool newBar = bar != lastBar;
			lastBar = bar;
			// REC: arm, cancel; LOOP: play / stop
			if (clearReq.exchange(false)) {
				hasLoop = playing = false;
				recState = R_IDLE;
			}
			if (recReq.exchange(false))
				recState = recState == R_IDLE ? R_ARMED : R_IDLE;
			if (loopReq.exchange(false) && hasLoop)
				playing = !playing;
			if (recState == R_ARMED && running && (newBar || resetHappened)) {
				recState = R_REC;
				recStart = (double) bar * 16.0;
				recLast = -1;
				playing = false;
			}
			resetHappened = false;

			bool hitNow = false;
			if (playing && hasLoop) {
				double lp = std::fmod(pos16 - anchor, (double) L16);
				if (lp < 0)
					lp += L16;
				double fi = lp / L16 * LOOP_N;
				int i0 = clamp((int) fi, 0, LOOP_N - 1), i1 = (i0 + 1) % LOOP_N;
				float f = (float) (fi - i0);
				float nx = lx[i0], ny = ly[i0];
				if (std::fabs(lx[i1] - nx) < 1.f && std::fabs(ly[i1] - ny) < 1.f) {
					nx += (lx[i1] - nx) * f;
					ny += (ly[i1] - ny) * f;
				}
				vx = (nx - px) / dt, vy = (ny - py) / dt;
				if (std::fabs(vx) > 50.f || std::fabs(vy) > 50.f)
					vx = vy = 0.f;
				px = nx, py = ny;
				if (i0 != lastPlayIdx) {
					// the hits on the way from the last point to this one
					int j = lastPlayIdx < 0 ? i0 : (lastPlayIdx + 1) % LOOP_N;
					for (int n = 0; n < LOOP_N; n++) {
						if (lhit[j])
							hitNow = true;
						if (j == i0)
							break;
						j = (j + 1) % LOOP_N;
					}
					lastPlayIdx = i0;
				}
				pubLoopPos = (float) (lp / L16);
			}
			else {
				lastPlayIdx = -1;
				physics(dt, grav, tilt, fric, mass, bnc, pushX, pushY, hitNow);
			}
			if (recState == R_REC) {
				double rp = pos16 - recStart;
				if (rp >= L16) { // done: the rest of the way, then play it
					for (int j = recLast + 1; j < LOOP_N; j++)
						lx[j] = px, ly[j] = py, lhit[j] = 0;
					hasLoop = true;
					playing = true;
					recState = R_IDLE;
					anchor = recStart;
					lastPlayIdx = -1;
				}
				else if (rp >= 0.0) {
					int idx = clamp((int) (rp / L16 * LOOP_N), 0, LOOP_N - 1);
					for (int j = recLast + 1; j <= idx; j++) {
						lx[j] = px, ly[j] = py;
						lhit[j] = j == idx && hitNow ? 1 : 0;
					}
					if (idx <= recLast && hitNow)
						lhit[idx] = 1;
					recLast = std::max(recLast, idx);
					pubRecPos = (float) (rp / L16);
				}
			}
			if (hitNow)
				hit();
			// the outputs' targets and their glide
			float spd = std::sqrt(vx * vx + vy * vy);
			tX = outV(0.5f * (px + 1.f));
			tY = outV(0.5f * (1.f - py));
			tS = outV(clamp(spd / 6.f, 0.f, 1.f));
			float sm = params[SMOOTH_PARAM].getValue();
			float tau = 0.0005f + sm * sm * 0.25f;
			slew = 1.f - std::exp(-args.sampleTime / tau);
			pubX = px, pubY = py;
			pubRec = recState;
			pubPlaying = playing && hasLoop;
			pubHasLoop = hasLoop;
			pubSync = sync;
			float bpm = 120.f;
			if (sync == S_FOLLOW && gi.stepPeriod && *gi.stepPeriod > 1.f)
				bpm = 60.f * args.sampleRate / (*gi.stepPeriod * 4.f);
			else if (sync == S_CLK && gap > 1.f)
				bpm = 60.f * args.sampleRate / (gap / per16 * 4.f);
			pubBpm = bpm;
		}
		oX += (tX - oX) * slew;
		oY += (tY - oY) * slew;
		oS += (tS - oS) * slew;
		outputs[X_OUTPUT].setVoltage(oX);
		outputs[Y_OUTPUT].setVoltage(oY);
		outputs[SPD_OUTPUT].setVoltage(oS);
		outputs[HIT_OUTPUT].setVoltage(hitP.process(args.sampleTime) ? 10.f : 0.f);
	}

	json_t* dataToJson() override {
		json_t* root = json_object();
		json_object_set_new(root, "version", json_integer(1));
		json_object_set_new(root, "preset", json_integer(preset));
		json_object_set_new(root, "wall", json_integer(wall.load()));
		json_object_set_new(root, "range", json_integer(range.load()));
		json_object_set_new(root, "loopLen", json_integer(loopLen.load()));
		json_object_set_new(root, "clockRes", json_integer(clockRes.load()));
		json_object_set_new(root, "hold", json_boolean(hold.load()));
		json_object_set_new(root, "follow", json_boolean(followOn.load()));
		json_object_set_new(root, "followId", json_integer(wantId));
		json_object_set_new(root, "kickFrom", json_integer(kickFrom.load()));
		json_object_set_new(root, "hasLoop", json_boolean(hasLoop));
		json_object_set_new(root, "playing", json_boolean(playing));
		if (hasLoop) {
			std::vector<int16_t> d((size_t) LOOP_N * 3);
			for (int i = 0; i < LOOP_N; i++) {
				d[i * 3] = (int16_t) std::lround(clamp(lx[i], -1.f, 1.f) * 32767.f);
				d[i * 3 + 1] = (int16_t) std::lround(clamp(ly[i], -1.f, 1.f) * 32767.f);
				d[i * 3 + 2] = lhit[i];
			}
			json_object_set_new(root, "loop", json_string(string::toBase64((const uint8_t*) d.data(), d.size() * sizeof(int16_t)).c_str()));
		}
		return root;
	}
	void dataFromJson(json_t* root) override {
		jsonCode++;
		if (json_t* j = json_object_get(root, "preset"))
			preset = clamp((int) json_integer_value(j), 0, NUM_PRESETS - 1);
		if (json_t* j = json_object_get(root, "wall"))
			wall = clamp((int) json_integer_value(j), 0, NUM_WALLS - 1);
		if (json_t* j = json_object_get(root, "range"))
			range = clamp((int) json_integer_value(j), 0, 2);
		if (json_t* j = json_object_get(root, "loopLen"))
			loopLen = clamp((int) json_integer_value(j), 0, 3);
		if (json_t* j = json_object_get(root, "clockRes"))
			clockRes = clamp((int) json_integer_value(j), 0, 3);
		hold = json_is_true(json_object_get(root, "hold"));
		followOn = !json_is_false(json_object_get(root, "follow"));
		if (json_t* j = json_object_get(root, "followId"))
			wantId = json_integer_value(j);
		if (json_t* j = json_object_get(root, "kickFrom"))
			kickFrom = clamp((int) json_integer_value(j), 0, NUM_KICKS - 1);
		bool has = false;
		json_t* jl = json_object_get(root, "loop");
		if (jl && json_string_value(jl)) {
			json_t* j = jl;
			std::vector<uint8_t> raw = string::fromBase64(json_string_value(j));
			if (raw.size() == (size_t) LOOP_N * 3 * sizeof(int16_t)) {
				const int16_t* d = (const int16_t*) raw.data();
				for (int i = 0; i < LOOP_N; i++) {
					lx[i] = d[i * 3] / 32767.f;
					ly[i] = d[i * 3 + 1] / 32767.f;
					lhit[i] = d[i * 3 + 2] ? 1 : 0;
				}
				has = json_is_true(json_object_get(root, "hasLoop"));
			}
		}
		hasLoop = has;
		playing = has && json_is_true(json_object_get(root, "playing"));
		recState = R_IDLE;
		uiTimer = 0.f;
	}
	void onReset() override {
		Module::onReset();
		loadPreset(0);
		range = 1;
		loopLen = 1;
		hold = false;
		hasLoop = playing = false;
		recState = R_IDLE;
		px = py = vx = vy = 0.f;
	}
};

// ================================================================ the panel
/** TILT: an endless knob (no pointer; the angle in the middle and as an arc). Drag up / right: clockwise. */
struct TiltKnob : app::ParamWidget {
	TiltKnob() { box.size = mm2px(Vec(9.6f, 9.6f)); }
	float acc = 0.f;
	float old = 0.f;
	void onDragStart(const DragStartEvent& e) override {
		if (e.button != GLFW_MOUSE_BUTTON_LEFT)
			return;
		if (engine::ParamQuantity* pq = getParamQuantity())
			old = pq->getValue();
	}
	void onDragMove(const DragMoveEvent& e) override {
		if (e.button != GLFW_MOUSE_BUTTON_LEFT)
			return;
		engine::ParamQuantity* pq = getParamQuantity();
		if (!pq)
			return;
		float d = (e.mouseDelta.x - e.mouseDelta.y) * 0.9f;
		int mods = APP->window->getMods();
		if ((mods & RACK_MOD_MASK) == RACK_MOD_CTRL)
			d *= 0.1f;
		pq->setValue(wrap360(pq->getValue() + d));
	}
	void onDragEnd(const DragEndEvent& e) override {
		if (e.button != GLFW_MOUSE_BUTTON_LEFT)
			return;
		engine::ParamQuantity* pq = getParamQuantity();
		if (!pq || pq->getValue() == old)
			return;
		history::ParamChange* h = new history::ParamChange;
		h->name = "move knob";
		h->moduleId = module ? module->id : -1;
		h->paramId = paramId;
		h->oldValue = old;
		h->newValue = pq->getValue();
		APP->history->push(h);
	}
	void onDoubleClick(const DoubleClickEvent& e) override {
		resetAction();
		e.consume(this);
	}
	void draw(const DrawArgs& args) override {
		NVGcontext* vg = args.vg;
		float r = box.size.x / 2;
		Vec c(r, r);
		nvgBeginPath(vg);
		nvgCircle(vg, c.x, c.y, r);
		nvgFillColor(vg, nvgRGB(0x0e, 0x0f, 0x12));
		nvgFill(vg);
		nvgBeginPath(vg);
		nvgCircle(vg, c.x, c.y, r * 0.86f);
		nvgFillColor(vg, nvgRGB(0x1d, 0x1f, 0x25));
		nvgFill(vg);
		for (int k = 0; k < 24; k++) {
			float a = k * 15.f * (float) M_PI / 180.f;
			nvgBeginPath(vg);
			nvgMoveTo(vg, c.x + std::sin(a) * r * 0.70f, c.y - std::cos(a) * r * 0.70f);
			nvgLineTo(vg, c.x + std::sin(a) * r * 0.80f, c.y - std::cos(a) * r * 0.80f);
			nvgStrokeColor(vg, nvgRGB(0x3a, 0x3c, 0x45));
			nvgStrokeWidth(vg, 0.9f);
			nvgStroke(vg);
		}
		ParamWidget::draw(args);
	}
	void drawLayer(const DrawArgs& args, int layer) override {
		if (layer == 1) {
			NVGcontext* vg = args.vg;
			float r = box.size.x / 2;
			float v = 0.f;
			if (engine::ParamQuantity* pq = getParamQuantity())
				v = wrap360(pq->getValue());
			float ar = r * 0.70f;
			nvgBeginPath(vg);
			nvgArc(vg, r, r, ar, -(float) M_PI / 2, -(float) M_PI / 2 + std::max(0.02f, v * (float) M_PI / 180.f), NVG_CW);
			nvgStrokeColor(vg, AC);
			nvgStrokeWidth(vg, 1.8f);
			nvgLineCap(vg, NVG_ROUND);
			nvgStroke(vg);
			tui::text(vg, tui::bold(), r, r + 0.5f, string::f("%d\xc2\xb0", (int) std::round(v) % 360), 7.f, tui::TXT, NVG_ALIGN_CENTER, 0.f);
		}
		ParamWidget::drawLayer(args, layer);
	}
};

/** The pad: the ball, its way, the direction of gravity; drag the ball and let go to throw it. */
struct PadView : widget::OpaqueWidget {
	TechnoKugel* mod = nullptr;
	struct Pt {
		float x, y;
	};
	std::vector<Pt> trail;
	struct Mv {
		double t;
		float x, y;
	};
	std::vector<Mv> moves;
	int seenHits = 0;
	float flash = 0.f;

	Vec toUnit(Vec local) const {
		return Vec(clamp(local.x / box.size.x * 2.f - 1.f, -1.f, 1.f), clamp(local.y / box.size.y * 2.f - 1.f, -1.f, 1.f));
	}
	Vec mouseLocal() {
		Vec m = APP->scene->rack->getMousePos();
		return m.minus(getRelativeOffset(Vec(0, 0), APP->scene->rack));
	}
	void grab(Vec u) {
		if (!mod)
			return;
		mod->dragX = u.x;
		mod->dragY = u.y;
		moves.push_back({system::getTime(), u.x, u.y});
		if (moves.size() > 16)
			moves.erase(moves.begin());
	}
	void onButton(const ButtonEvent& e) override {
		if (e.button == GLFW_MOUSE_BUTTON_LEFT && e.action == GLFW_PRESS && mod) {
			moves.clear();
			grab(toUnit(e.pos));
			mod->dragOn = true;
			e.consume(this);
		}
		// other buttons go on to the module (right-click: its menu)
	}
	void onDragMove(const DragMoveEvent& e) override {
		if (mod && mod->dragOn.load())
			grab(toUnit(mouseLocal()));
	}
	void onDragEnd(const DragEndEvent& e) override {
		if (!mod || !mod->dragOn.load())
			return;
		// the throw: the way of the last ~80 ms
		double now = system::getTime();
		float vx = 0.f, vy = 0.f;
		if (!moves.empty()) {
			const Mv& last = moves.back();
			const Mv* from = &moves.front();
			for (const Mv& m : moves)
				if (now - m.t <= 0.08) {
					from = &m;
					break;
				}
			double dt = std::max(0.016, last.t - from->t);
			if (now - last.t < 0.1) {
				vx = (float) ((last.x - from->x) / dt);
				vy = (float) ((last.y - from->y) / dt);
			}
		}
		float sp = std::sqrt(vx * vx + vy * vy);
		if (sp > 14.f)
			vx *= 14.f / sp, vy *= 14.f / sp;
		mod->throwVx = vx;
		mod->throwVy = vy;
		mod->throwReq = true;
		mod->dragOn = false;
	}
	void step() override {
		OpaqueWidget::step();
		if (!mod)
			return;
		trail.push_back({mod->pubX.load(), mod->pubY.load()});
		if (trail.size() > 70)
			trail.erase(trail.begin());
		int h = mod->pubHits.load();
		if (h != seenHits) {
			seenHits = h;
			flash = 1.f;
		}
		flash = std::max(0.f, flash - 0.06f);
	}
	void drawLayer(const DrawArgs& args, int layer) override {
		if (layer != 1)
			return OpaqueWidget::drawLayer(args, layer);
		NVGcontext* vg = args.vg;
		float w = box.size.x, hh = box.size.y;
		nvgSave(vg);
		nvgScissor(vg, 0, 0, w, hh);
		// the grid
		for (int i = 1; i < 4; i++) {
			nvgBeginPath(vg);
			nvgMoveTo(vg, w * i / 4, 3);
			nvgLineTo(vg, w * i / 4, hh - 3);
			nvgMoveTo(vg, 3, hh * i / 4);
			nvgLineTo(vg, w - 3, hh * i / 4);
			nvgStrokeColor(vg, nvgRGB(0x1f, 0x2a, 0x31));
			nvgStrokeWidth(vg, 0.7f);
			nvgStroke(vg);
		}
		int wl = mod ? mod->wall.load() : W_BOUNCE;
		NVGcolor edge = tui::mixc(tui::DISP, AC, flash * 0.8f + 0.15f);
		nvgBeginPath(vg);
		if (wl == W_ROUND)
			nvgCircle(vg, w / 2, hh / 2, w / 2 - 1.5f);
		else
			nvgRect(vg, 1.5f, 1.5f, w - 3, hh - 3);
		nvgStrokeColor(vg, edge);
		nvgStrokeWidth(vg, wl == W_WRAP ? 0.6f : 1.4f);
		if (wl == W_WRAP)
			nvgStrokeColor(vg, tui::mixc(tui::DISP, AC, 0.25f));
		nvgStroke(vg);
		// the direction of gravity
		if (mod && mod->eff[0].load() > 0.001f) {
			float th = mod->eff[1].load() * (float) M_PI / 180.f;
			float dx = -std::sin(th), dy = std::cos(th);
			float cx = w / 2, cy = hh / 2, L = 8.f;
			nvgBeginPath(vg);
			nvgMoveTo(vg, cx - dx * L, cy - dy * L);
			nvgLineTo(vg, cx + dx * L, cy + dy * L);
			nvgMoveTo(vg, cx + dx * L - dx * 3.5f - dy * 3.f, cy + dy * L - dy * 3.5f + dx * 3.f);
			nvgLineTo(vg, cx + dx * L, cy + dy * L);
			nvgLineTo(vg, cx + dx * L - dx * 3.5f + dy * 3.f, cy + dy * L - dy * 3.5f - dx * 3.f);
			nvgStrokeColor(vg, nvgRGBA(0x8a, 0x8d, 0x99, 120));
			nvgStrokeWidth(vg, 1.0f);
			nvgLineCap(vg, NVG_ROUND);
			nvgStroke(vg);
		}
		auto P = [&](float x, float y) { return Vec((x + 1.f) * 0.5f * w, (y + 1.f) * 0.5f * hh); };
		// the recorded loop (when it plays: its whole way, faint)
		if (mod && mod->pubHasLoop.load()) {
			nvgBeginPath(vg);
			for (int i = 0; i < LOOP_N; i += 4) {
				Vec p = P(mod->lx[i], mod->ly[i]);
				if (i == 0 || (i >= 4 && (std::fabs(mod->lx[i] - mod->lx[i - 4]) > 1.f || std::fabs(mod->ly[i] - mod->ly[i - 4]) > 1.f)))
					nvgMoveTo(vg, p.x, p.y);
				else
					nvgLineTo(vg, p.x, p.y);
			}
			nvgStrokeColor(vg, mod->pubPlaying.load() ? nvgRGBA(0x2f, 0xb8, 0x66, 70) : nvgRGBA(0x8a, 0x8d, 0x99, 40));
			nvgStrokeWidth(vg, 0.8f);
			nvgStroke(vg);
		}
		// the way
		for (size_t i = 1; i < trail.size(); i++) {
			if (std::fabs(trail[i].x - trail[i - 1].x) > 1.f || std::fabs(trail[i].y - trail[i - 1].y) > 1.f)
				continue;
			Vec a = P(trail[i - 1].x, trail[i - 1].y), b = P(trail[i].x, trail[i].y);
			nvgBeginPath(vg);
			nvgMoveTo(vg, a.x, a.y);
			nvgLineTo(vg, b.x, b.y);
			nvgStrokeColor(vg, nvgRGBAf(AC.r, AC.g, AC.b, 0.5f * i / trail.size()));
			nvgStrokeWidth(vg, 1.3f);
			nvgLineCap(vg, NVG_ROUND);
			nvgStroke(vg);
		}
		// the ball
		Vec b = P(mod ? mod->pubX.load() : 0.68f, mod ? mod->pubY.load() : 0.24f);
		nvgBeginPath(vg);
		nvgCircle(vg, b.x, b.y, 7.5f + flash * 2.f);
		nvgFillColor(vg, nvgRGBAf(AC.r, AC.g, AC.b, 0.22f + 0.2f * flash));
		nvgFill(vg);
		nvgBeginPath(vg);
		nvgCircle(vg, b.x, b.y, 4.8f);
		nvgFillColor(vg, mod && mod->hold.load() ? tui::mixc(AC, tui::WHITE, 0.5f) : AC);
		nvgFill(vg);
		nvgRestore(vg);
	}
};

struct TechnoKugelWidget : ModuleWidget {
	TechnoKugel* mod = nullptr;
	bool mapped = false;
	float glow = 0.f;

	TechnoKugelWidget(TechnoKugel* module) {
		setModule(module);
		mod = module;
		setPanel(createPanel(asset::plugin(pluginInstance, "res/TechnoKugel.svg")));
		using namespace tui;
		addParam(createParamCentered<GLargeKnob>(mm(L::K_GRAV), module, TechnoKugel::GRAV_PARAM));
		addParam(createParamCentered<TiltKnob>(mm(L::K_TILT), module, TechnoKugel::TILT_PARAM));
		addParam(createParamCentered<GKnob>(mm(L::K_FRIC), module, TechnoKugel::FRIC_PARAM));
		addParam(createParamCentered<GSmallKnob>(mm(L::K_MASS), module, TechnoKugel::MASS_PARAM));
		addParam(createParamCentered<GSmallKnob>(mm(L::K_BNC), module, TechnoKugel::BNC_PARAM));
		addParam(createParamCentered<GSmallKnob>(mm(L::K_FORCE), module, TechnoKugel::FORCE_PARAM));
		addParam(createParamCentered<GSmallKnob>(mm(L::K_SMOOTH), module, TechnoKugel::SMOOTH_PARAM));

		static const float* const IN[TechnoKugel::NUM_INPUTS] = {L::IN_GATE, L::IN_VEL, L::IN_CLK, L::IN_RST, L::IN_GRAV, L::IN_TILT,
		                                                         L::IN_FRIC, L::IN_BNC, L::IN_PUSHX, L::IN_PUSHY, L::IN_REC};
		for (int i = 0; i < TechnoKugel::NUM_INPUTS; i++)
			addInput(createInputCentered<PJ301MPort>(mm(IN[i]), module, i));
		static const float* const OUT[TechnoKugel::NUM_OUTPUTS] = {L::OUT_X, L::OUT_Y, L::OUT_SPD, L::OUT_HIT};
		for (int i = 0; i < TechnoKugel::NUM_OUTPUTS; i++)
			addOutput(createOutputCentered<PJ301MPort>(mm(OUT[i]), module, i));

		// the preset and the three fields
		TField* pr = place<TField>(this, L::F_PRESET);
		if (module)
			pr->mapSelect(module, TechnoKugel::PRESETSEL_PARAM, &module->jsonCode);
		pr->prefix = "PRESET";
		pr->col = AC;
		pr->value = [=]() { return std::string(PRESETS[mod ? mod->preset : 0].name); };
		pr->count = []() { return NUM_PRESETS; };
		pr->current = [=]() { return mod ? mod->preset : 0; };
		pr->item = [](int i) { return std::string(PRESETS[i].name); };
		pr->choose = [=](int i) {
			if (mod)
				mod->loadPreset(i);
		};
		TField* fw = place<TField>(this, L::F_WALL);
		fw->col = AC;
		fw->value = [=]() { return std::string(WALL_NAMES[mod ? mod->wall.load() : 0]); };
		fw->count = []() { return (int) NUM_WALLS; };
		fw->current = [=]() { return mod ? mod->wall.load() : 0; };
		fw->item = [](int i) { return std::string(WALL_TIPS[i]); };
		fw->choose = [=](int i) {
			if (mod)
				mod->wall = i;
		};
		TField* fr = place<TField>(this, L::F_RANGE);
		fr->col = AC;
		fr->value = [=]() { return std::string(RANGE_NAMES[mod ? mod->range.load() : 1]); };
		fr->count = []() { return 3; };
		fr->current = [=]() { return mod ? mod->range.load() : 1; };
		fr->item = [](int i) { return std::string(RANGE_NAMES[i]); };
		fr->choose = [=](int i) {
			if (mod)
				mod->range = i;
		};
		TField* fl = place<TField>(this, L::F_LOOP);
		fl->col = AC;
		fl->value = [=]() { return std::string(LOOP_NAMES[mod ? mod->loopLen.load() : 1]); };
		fl->count = []() { return 4; };
		fl->current = [=]() { return mod ? mod->loopLen.load() : 1; };
		fl->item = [](int i) { return std::string(LOOP_NAMES[i]); };
		fl->choose = [=](int i) {
			if (mod)
				mod->loopLen = i;
		};

		// the keys (colours as in all GENRA TECHNO modules: orange makes something new, blue a mode, red records,
		// green plays)
		TButton* kick = place<TButton>(this, L::BTN_KICK);
		kick->label = "KICK";
		kick->col = dark(NEWCOL, 0.75f);
		kick->textCol = WHITE;
		kick->act = [=]() {
			if (mod)
				mod->kickReq = true;
		};
		TButton* hold = place<TButton>(this, L::BTN_HOLD);
		hold->label = "HOLD";
		hold->lit = [=]() { return mod && mod->hold.load(); };
		hold->colOf = [=]() { return mod && mod->hold.load() ? MODECOL : BTN; };
		hold->act = [=]() {
			if (mod)
				mod->hold = !mod->hold.load();
		};
		TButton* rec = place<TButton>(this, L::BTN_REC);
		rec->labelOf = [=]() {
			int s = mod ? mod->pubRec.load() : 0;
			return std::string(s == TechnoKugel::R_ARMED ? "REC …" : "REC");
		};
		rec->textCol = WHITE;
		rec->lit = [=]() { return mod && mod->pubRec.load() == TechnoKugel::R_REC; };
		rec->colOf = [=]() {
			int s = mod ? mod->pubRec.load() : 0;
			if (s == TechnoKugel::R_REC)
				return RED;
			if (s == TechnoKugel::R_ARMED)
				return std::fmod(system::getTime(), 0.5) < 0.25 ? RED : dark(RED, 0.4f);
			return dark(RED, 0.55f);
		};
		rec->act = [=]() {
			if (mod)
				mod->recReq = true;
		};
		rec->rightMenu = [=](ui::Menu* m) {
			m->addChild(createMenuItem("Clear the loop", "", [=]() {
				if (mod)
					mod->clearReq = true;
			}));
		};
		TButton* loop = place<TButton>(this, L::BTN_LOOP);
		loop->label = "LOOP";
		loop->textCol = WHITE;
		loop->enabled = [=]() { return mod && mod->pubHasLoop.load(); };
		loop->lit = [=]() { return mod && mod->pubPlaying.load(); };
		loop->colOf = [=]() { return mod && mod->pubPlaying.load() ? PLAYCOL : dark(PLAYCOL, 0.45f); };
		loop->act = [=]() {
			if (mod)
				mod->loopReq = true;
		};

		PadView* pad = place<PadView>(this, L::DISP_PAD);
		pad->mod = module;
		Drawn* info = place<Drawn>(this, L::DISP_INFO);
		info->paintLit = [=](const DrawArgs& a) { drawInfo(a.vg, info->box.size); };
		Drawn* ln = place<Drawn>(this, L::A_LINK);
		ln->paintLit = [=](const DrawArgs& a) {
			bool on = mod && mod->pubSync.load() == TechnoKugel::S_FOLLOW;
			nvgBeginPath(a.vg);
			nvgCircle(a.vg, ln->box.size.x / 2, ln->box.size.y / 2 - 3, 3.5f);
			nvgFillColor(a.vg, on ? GREEN : nvgRGB(0x44, 0x46, 0x50));
			nvgFill(a.vg);
			text(a.vg, bold(), ln->box.size.x / 2, ln->box.size.y - 3, "LINK", 6.5f, DIM, NVG_ALIGN_CENTER);
		};
		// the lit segments over GATE, VEL, CLK, RST while the GENRA feeds them (no cable)
		static const float* const SEG[4] = {L::IN_GATE, L::IN_VEL, L::IN_CLK, L::IN_RST};
		for (int k = 0; k < 4; k++) {
			Drawn* s = new Drawn;
			s->box.pos = mm2px(Vec(SEG[k][0] - 3.f, SEG[k][1] - 4.6f));
			s->box.size = mm2px(Vec(6.f, 0.5f));
			addChild(s);
			s->paintLit = [=](const DrawArgs& a) {
				if (!mod || !mod->pubFollowFeeds.load())
					return;
				int in = k == 0 ? TechnoKugel::GATE_INPUT : (k == 1 ? TechnoKugel::VEL_INPUT : (k == 2 ? TechnoKugel::CLK_INPUT : TechnoKugel::RST_INPUT));
				if (mod->inputs[in].isConnected())
					return;
				if (k < 2 && mod->kickFrom.load() == K_NONE)
					return;
				if (k == 1 && mod->inputs[TechnoKugel::GATE_INPUT].isConnected())
					return;
				if (k == 3 && mod->inputs[TechnoKugel::CLK_INPUT].isConnected())
					return;
				roundRect(a.vg, 0, 0, s->box.size.x, s->box.size.y, 1.f, k < 2 ? AC : WHITE);
			};
		}
	}

	void drawInfo(NVGcontext* vg, Vec sz) {
		using namespace tui;
		auto f = bold();
		TechnoKugel* m = mod;
		float g = m ? m->eff[0].load() : 0.31f, t = m ? m->eff[1].load() : 20.f, fr = m ? m->eff[2].load() : 0.18f;
		float ms = m ? m->eff[3].load() : 1.f, bn = m ? m->eff[4].load() : 0.74f;
		std::string loop = "OFF";
		if (m) {
			int L16 = LOOP_BARS[clamp(m->loopLen.load(), 0, 3)] * 16;
			int bars = L16 / 16;
			int rs = m->pubRec.load();
			if (rs == TechnoKugel::R_ARMED)
				loop = "REC ON NEXT BAR";
			else if (rs == TechnoKugel::R_REC)
				loop = string::f("REC %d / %d", std::min(bars, (int) (m->pubRecPos.load() * bars) + 1), bars);
			else if (m->pubPlaying.load())
				loop = string::f("PLAY %d / %d", std::min(bars, (int) (m->pubLoopPos.load() * bars) + 1), bars);
			else if (m->pubHasLoop.load())
				loop = "STOPPED";
		}
		std::string sync = "OWN 120 BPM";
		if (m) {
			int s = m->pubSync.load();
			if (s == TechnoKugel::S_CLK)
				sync = string::f("CLK %.0f BPM", m->pubBpm.load());
			else if (s == TechnoKugel::S_FOLLOW)
				sync = string::f("GENRA %.0f BPM", m->pubBpm.load());
		}
		float vx = m ? m->outputs[TechnoKugel::X_OUTPUT].getVoltage() : 2.84f, vy = m ? m->outputs[TechnoKugel::Y_OUTPUT].getVoltage() : -3.1f;
		struct Row {
			std::string k, v;
			bool hot;
		};
		std::vector<Row> rows = {
		    {"GRAVITY", string::f("%.2f g", g * 2.f), false},
		    {"TILT", string::f("%d\xc2\xb0", (int) std::round(t) % 360), false},
		    {"FRICTION", string::f("%d %%", (int) std::round(fr * 100.f)), false},
		    {"MASS", string::f("%.1f kg", ms), false},
		    {"BOUNCE", string::f("%d %%", (int) std::round(bn * 100.f)), false},
		    {"WALL", WALL_NAMES[m ? m->wall.load() : 0], false},
		    {"LOOP", loop, m && (m->pubRec.load() != TechnoKugel::R_IDLE)},
		    {"SYNC", sync, false},
		    {"X", string::f("%+.2f V", vx), true},
		    {"Y", string::f("%+.2f V", vy), true},
		};
		float x0 = 9.f, x1 = sz.x - 9.f;
		float rh = (sz.y - 10.f) / (rows.size() + 1);
		for (size_t i = 0; i < rows.size(); i++) {
			float y = 7.f + rh * (i + 0.5f);
			text(vg, f, x0, y, rows[i].k, 6.6f, DIM, NVG_ALIGN_LEFT, 0.6f);
			NVGcolor c = rows[i].hot ? (rows[i].k == "LOOP" ? RED : AC) : TXT;
			textFit(vg, f, x1, y, rows[i].v, 7.4f, x1 - x0 - 50.f, c, NVG_ALIGN_RIGHT, 0.4f);
		}
		float y = 7.f + rh * (rows.size() + 0.5f);
		text(vg, f, x0, y, "HIT", 6.6f, DIM, NVG_ALIGN_LEFT, 0.6f);
		bool hitOn = m && m->outputs[TechnoKugel::HIT_OUTPUT].getVoltage() > 1.f;
		glow = hitOn ? 1.f : std::max(0.f, glow - 0.08f);
		nvgBeginPath(vg);
		nvgCircle(vg, x1 - 3.f, y, 3.f);
		nvgFillColor(vg, mixc(nvgRGB(0x22, 0x24, 0x2b), AC, m ? glow : 1.f));
		nvgFill(vg);
	}

	void step() override {
		ModuleWidget::step();
		if (!mapped) {
			tui::mapButtons(this, module, TechnoKugel::BTN_PARAM, BTNS, NUM_BTNS);
			if (mod)
				tui::mapFields(this, module, TechnoKugel::FLD_PARAM, FLDS, NUM_FLDS, &mod->jsonCode);
			mapped = true;
		}
		if (mod)
			mod->uiStep(APP->window->getLastFrameDuration());
	}

	void appendContextMenu(Menu* menu) override {
		TechnoKugel* m = mod;
		if (!m)
			return;
		menu->addChild(new MenuSeparator);
		menu->addChild(createSubmenuItem("Follow GENRA", m->followOn.load() ? (m->src.to.load() ? "on" : "none found") : "off", [=](Menu* sub) {
			sub->addChild(createCheckMenuItem("Off", "", [=]() { return !m->followOn.load(); }, [=]() {
				m->followOn = false;
				m->uiTimer = 0.f;
			}));
			sub->addChild(createCheckMenuItem("Automatic (the first GENRA)", "", [=]() { return m->followOn.load() && m->wantId < 0; }, [=]() {
				m->followOn = true;
				m->wantId = -1;
				m->uiTimer = 0.f;
			}));
			int n = 0;
			for (auto& s : TechnoKugel::sources(m)) {
				int64_t id = s.first;
				sub->addChild(createCheckMenuItem(string::f("%s #%d", s.second.c_str(), ++n), "",
				                                  [=]() { return m->followOn.load() && m->wantId == id; }, [=]() {
					                                  m->followOn = true;
					                                  m->wantId = id;
					                                  m->uiTimer = 0.f;
				                                  }));
			}
			sub->addChild(new MenuSeparator);
			sub->addChild(createMenuLabel("A cable in CLK / GATE always wins"));
		}));
		menu->addChild(createSubmenuItem("Kick from (GENRA's track)", KICK_NAMES[m->kickFrom.load()], [=](Menu* sub) {
			for (int i = 0; i < NUM_KICKS; i++)
				sub->addChild(createCheckMenuItem(KICK_NAMES[i], "", [=]() { return m->kickFrom.load() == i; }, [=]() { m->kickFrom = i; }));
		}));
		menu->addChild(createSubmenuItem("Clock input (CLK jack)", CLK_RES_NAMES[m->clockRes.load()], [=](Menu* sub) {
			for (int i = 0; i < 4; i++)
				sub->addChild(createCheckMenuItem(CLK_RES_NAMES[i], "", [=]() { return m->clockRes.load() == i; }, [=]() { m->clockRes = i; }));
		}));
		menu->addChild(createMenuItem("Clear the loop", "", [=]() { m->clearReq = true; }));
	}
};

Model* modelTechnoKugel = createModel<TechnoKugel, TechnoKugelWidget>("TechnoKugel");
