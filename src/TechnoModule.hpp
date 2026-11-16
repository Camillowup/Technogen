// SPDX-License-Identifier: GPL-3.0-only
// GENRA TECHNO, the main module: the generator and the player. GRID, ABLAUF and MOD find it through the registry below
// (right-click › Connect to …, automatic: the first one) and read what it publishes; they never keep a pointer past its
// removal (TechnoLink: onRemove of the main module clears every link to it, under the engine lock).
//
// Threads: the pattern is generated on the UI thread (the panel's step()), handed over through `incoming` and taken
// on at the next bar (or at once when stopped) by the audio thread. GRID writes single notes straight into `pat`
// (plain bytes, a torn read costs one note for one 16th). The song: LIVE (the module's own plan, `plans[planCur]`) or
// SONG (ABLAUF's script, `script`).
#pragma once
#include "plugin.hpp"
#include "tg/Engine.hpp"
#include "link/Hub.hpp"
#include "link/GenraPorts.hpp"
#include <atomic>
#include <mutex>
#include <deque>

struct GenraTechno;

/** A module's link to a GENRA TECHNO (GRID, ABLAUF, MOD). */
struct TechnoLink {
	std::atomic<GenraTechno*> main{nullptr};
	int64_t wantId = -1; // -1: automatic (the first GENRA TECHNO)
	float timer = 0.f;
	Module* owner = nullptr;
	TechnoLink();
	~TechnoLink();
	/** UI thread: find the GENRA TECHNO (again). */
	void uiStep(float dt);
	void appendMenu(ui::Menu* menu);
	void toJson(json_t* root);
	void fromJson(json_t* root);
};

namespace techno_registry {
void add(GenraTechno* g);
/** Engine lock held (onRemove): every link to g is cleared. */
void remove(GenraTechno* g);
std::vector<GenraTechno*> all();
} // namespace techno_registry

/** ABLAUF's song as the main module plays it (immutable once handed over). */
struct SongScript {
	std::vector<tg::Section> rows;
	std::vector<tg::Pattern> pads; // indexed by Section::pad
	bool djEdges = true;
	int loopRow = -1;              // ABLAUF's LOOP: this row again and again
};

/** A published copy of the song for the displays (UI thread). */
struct SongView {
	std::vector<tg::Section> sections;
	bool fromAblauf = false;
};

struct GenraTechno : Module {
	enum ParamId {
		BPM_PARAM,
		SWING_PARAM,
		ENERGY_PARAM,
		EVOLVE_PARAM,
		LENGTH_PARAM,
		ENUMS(DENS_PARAM, 5),
		ENUMS(FLAV_PARAM, 5),
		ENUMS(LEN_PARAM, 5),
		NUM_PARAMS
	};
	enum InputId { CLK_INPUT, RST_INPUT, RUN_INPUT, NEW_INPUT, BUILD_INPUT, DROP_INPUT, ENERGY_INPUT, EVOLVE_INPUT, NUM_INPUTS };
	enum OutputId {
		ENUMS(DRUM_OUTPUT, 8),
		CLK_OUTPUT,
		RST_OUTPUT,
		BAR_OUTPUT,
		ENUMS(BASS_OUTPUT, 5),
		ENUMS(STAB_OUTPUT, 3),
		ENUMS(LEAD_OUTPUT, 5),
		ENUMS(ATMO_OUTPUT, 3),
		DUCK_OUTPUT,
		RISE_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightId { NUM_LIGHTS };

	// ---------------------------------------------------------------- what is saved
	tg::Pattern pat;
	int style = 2, key = 9, forcedBuild = tg::B_AUTO;
	int typeSel[tg::NUM_TRACKS] = {-1, -1, -1, -1, -1}; // -1: the style's
	bool lock[tg::NUM_TRACKS] = {}, mute[tg::NUM_TRACKS] = {};
	std::atomic<int> solo{-1}; // GRID's SOLO: only this track
	uint32_t songSeed = 1;

	// ---------------------------------------------------------------- UI → audio
	std::atomic<tg::Pattern*> incoming{nullptr};
	std::atomic<bool> incomingNow{false}; // take it at once (edits), else at the next bar
	std::atomic<tg::Pattern*> spent{nullptr};
	struct Plan {
		tg::Section s[96];
		int n = 0;
	};
	Plan plans[2];
	std::atomic<int> planCur{0};
	std::atomic<SongScript*> script{nullptr}; // ABLAUF, SONG mode (nullptr: LIVE)
	std::atomic<bool> runReq{false}, stopReq{false};
	std::atomic<int> startRow{-1};  // ABLAUF › PLAY from this row
	std::atomic<int> performReq{-1}; // BUILD / DROP / BREAK (Part) at the next phrase
	std::atomic<bool> fillReq{false};
	std::atomic<int> performHold{-1}; // the part button held down
	std::atomic<bool> nextRowReq{false};

	// ---------------------------------------------------------------- audio → UI (requests and what the displays show)
	std::atomic<int> newReq{0};    // bit t: a new track t (bit 5: all), from the NEW input
	std::atomic<int> mutateReq{0}; // bit t: EVOLVE
	std::atomic<bool> running{false};
	std::atomic<int> pubPart{tg::P_INTRO}, pubSecBar{0}, pubSecBars{32}, pubSecIdx{0}, pubNextPart{tg::P_GROOVE},
	    pubBuild{tg::BT_NONE}, pubDropIdx{0}, pubGrow{0}, pubSongBar{0}, pubRow{-1}, pubChordCount{0};
	std::atomic<int64_t> pubBarCount{0}, pubSongStep{0};
	std::atomic<float> pubSecPos{0.f}, pubRise{0.f};
	std::atomic<bool> pubSongMode{false};
	std::atomic<int> pubStepInBar{0};

	// the displays' copy of the song (UI thread only)
	std::mutex viewMx;
	SongView view;

	// ---------------------------------------------------------------- the player (audio thread)
	dsp::SchmittTrigger clkTrig, rstTrig, runTrig, newTrig, buildTrig, dropTrig;
	dsp::PulseGenerator clkPulse, rstPulse, barPulse;
	float phase = 0.f, stepPeriod = 5000.f, sinceTick = 0.f, clockGap = 0.f, sinceClock = 0.f;
	bool extClockSeen = false;
	int64_t songStep = 0;
	tg::Section cur;
	int planIdx = 0, secBar = 0, row = 0, dropsSoFar = 0, buildsSoFar = 0;
	tg::Section ov[4];
	int ovN = 0, ovI = 0;
	bool inOv = false;
	int lastChordDeg = -1;
	float duck = 0.f, riseSm = 0.f;
	struct DrumV {
		float trig = 0.f, vel = 0.f, delay = -1.f, ratInterval = 0.f, ratCounter = 0.f;
		int ratLeft = 0;
		float pVel = 0.f;
		int pRat = 1;
	} drums[8];
	struct MelV {
		float delay = -1.f;         // samples until the note starts
		tg::Note pending;
		int pendingOct = 0;
		float gate = 0.f;           // samples left of the gate
		float retrig = 0.f;         // a short low before a new note
		float voct[4] = {}, vel = 0.f;
		int n = 1;
		bool acc = false, slide = false;
		int ratLeft = 0;
		float ratInterval = 0.f, ratCounter = 0.f, ratGate = 0.f;
	} mels[4];
	bool fillBar = false, wasSong = false;

	// FOLLOW: GENRA's output layout for the instruments of the GENRA plugins (link hub)
	enum { FOLLOW_OUTS = genraports::RESET_OUTPUT + 1 };
	float followPub[2][FOLLOW_OUTS][genraports::PUB_CHANNELS] = {};

	GenraTechno();
	~GenraTechno();
	void onRemove(const RemoveEvent& e) override;
	void onReset(const ResetEvent& e) override;
	void process(const ProcessArgs& args) override;
	json_t* dataToJson() override;
	void dataFromJson(json_t* root) override;

	// ---- the song
	int planN() { return plans[planCur.load()].n; }
	void startSong(int fromRow);
	void enterSection(const tg::Section& s);
	void advanceSection();
	void tick(float sr);
	void fireStep(float sr);
	void publishFollow(int64_t frame);

	// ---- UI thread
	tg::GenSettings settings();
	void post(const tg::Pattern& p, bool now);
	void rebuildPlan();
	void updateView();
	float bpm();
	int effectiveType(int t) { return typeSel[t] >= 0 ? typeSel[t] : tg::styleType(style, t); }
};
