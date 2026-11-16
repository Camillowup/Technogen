// SPDX-License-Identifier: GPL-3.0-only
// GENRA TECHNO, the main module (see TechnoModule.hpp).
#include "TechnoModule.hpp"
#include "TechnoUI.hpp"
#include "GenraTechnoLayout.hpp"
#include "MidiFolder.hpp"
#include <algorithm>
#include <cmath>

using namespace tg;
namespace L = genratechno_layout;

// ================================================================ the registry and the links
namespace techno_registry {
static std::mutex mx;
static std::vector<GenraTechno*> mains;
static std::vector<TechnoLink*> links;
void add(GenraTechno* g) {
	std::lock_guard<std::mutex> lk(mx);
	mains.push_back(g);
}
void remove(GenraTechno* g) {
	std::lock_guard<std::mutex> lk(mx);
	mains.erase(std::remove(mains.begin(), mains.end(), g), mains.end());
	for (TechnoLink* l : links)
		if (l->main.load() == g)
			l->main.store(nullptr);
}
std::vector<GenraTechno*> all() {
	std::lock_guard<std::mutex> lk(mx);
	return mains;
}
static void addLink(TechnoLink* l) {
	std::lock_guard<std::mutex> lk(mx);
	links.push_back(l);
}
static void removeLink(TechnoLink* l) {
	std::lock_guard<std::mutex> lk(mx);
	links.erase(std::remove(links.begin(), links.end(), l), links.end());
}
} // namespace techno_registry

TechnoLink::TechnoLink() { techno_registry::addLink(this); }
TechnoLink::~TechnoLink() { techno_registry::removeLink(this); }

void TechnoLink::uiStep(float dt) {
	timer -= dt;
	if (timer > 0.f)
		return;
	timer = 0.5f;
	GenraTechno* want = nullptr;
	std::vector<GenraTechno*> all = techno_registry::all();
	for (GenraTechno* g : all)
		if (wantId < 0 || g->id == wantId) {
			want = g;
			break;
		}
	if (want != main.load())
		main.store(want);
}

void TechnoLink::appendMenu(ui::Menu* menu) {
	menu->addChild(new ui::MenuSeparator);
	menu->addChild(createSubmenuItem("Connect to", "", [=](ui::Menu* sub) {
		sub->addChild(createCheckMenuItem("Automatic (the first GENRA TECHNO)", "", [=]() { return wantId < 0; },
		                                  [=]() {
			                                  wantId = -1;
			                                  timer = 0.f;
		                                  }));
		int n = 0;
		for (GenraTechno* g : techno_registry::all()) {
			int64_t id = g->id;
			sub->addChild(createCheckMenuItem(string::f("GENRA TECHNO #%d", ++n), "", [=]() { return wantId == id; },
			                                  [=]() {
				                                  wantId = id;
				                                  timer = 0.f;
			                                  }));
		}
	}));
}
void TechnoLink::toJson(json_t* root) { json_object_set_new(root, "link", json_integer(wantId)); }
void TechnoLink::fromJson(json_t* root) {
	if (json_t* j = json_object_get(root, "link"))
		wantId = json_integer_value(j);
}

// ================================================================ the module
GenraTechno::GenraTechno() {
	config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	configParam(BPM_PARAM, 60.f, 200.f, 132.f, "Tempo", " BPM");
	configParam(SWING_PARAM, 50.f, 62.f, 51.f, "Swing", " %");
	configParam(ENERGY_PARAM, 0.f, 1.f, 0.6f, "Energy", " %", 0.f, 100.f);
	configParam(EVOLVE_PARAM, 0.f, 1.f, 0.2f, "Evolve (a small change every 8 bars)", " %", 0.f, 100.f);
	configParam(LENGTH_PARAM, 2.f, 12.f, 6.f, "Song length (LIVE)", " min");
	static const char* const flav[5] = {"Groove (swing feel, ghosts)", "Acid (slides and accents)", "Dub (echo hits)",
	                                    "Loop (length of the loop)", "Move (the MOD output)"};
	for (int t = 0; t < NUM_TRACKS; t++) {
		configParam(DENS_PARAM + t, 0.f, 1.f, 0.5f, std::string(TRACK_NAMES[t]) + " density", " %", 0.f, 100.f);
		configParam(FLAV_PARAM + t, 0.f, 1.f, 0.5f, std::string(TRACK_NAMES[t]) + " " + flav[t], " %", 0.f, 100.f);
		configParam(LEN_PARAM + t, 0.f, 64.f, 0.f, std::string(TRACK_NAMES[t]) + " loop length (0: the pattern's)", " 16ths");
		paramQuantities[LEN_PARAM + t]->snapEnabled = true;
	}
	configInput(CLK_INPUT, "Clock (16ths)");
	configInput(RST_INPUT, "Reset");
	configInput(RUN_INPUT, "Run (trigger: start / stop)");
	configInput(NEW_INPUT, "NEW ALL (trigger)");
	configInput(BUILD_INPUT, "BUILD (trigger)");
	configInput(DROP_INPUT, "DROP (trigger)");
	configInput(ENERGY_INPUT, "Energy CV (±5 V adds ±50 %)");
	configInput(EVOLVE_INPUT, "Evolve CV (±5 V adds ±50 %)");
	for (int v = 0; v < 8; v++)
		configOutput(DRUM_OUTPUT + v, std::string(DRUM_NAMES[v]) + " trigger (level = velocity)");
	configOutput(CLK_OUTPUT, "Clock (16ths)");
	configOutput(RST_OUTPUT, "Reset");
	configOutput(BAR_OUTPUT, "Bar (a trigger on every 1)");
	static const char* const mono[5] = {"V/OCT", "Gate", "Velocity", "Accent", "Slide"};
	for (int i = 0; i < 5; i++) {
		configOutput(BASS_OUTPUT + i, std::string("Bass ") + mono[i]);
		configOutput(LEAD_OUTPUT + i, std::string("Lead ") + mono[i]);
	}
	configOutput(STAB_OUTPUT + 0, "Stab V/OCT (poly 4)");
	configOutput(STAB_OUTPUT + 1, "Stab gate (poly 4)");
	configOutput(STAB_OUTPUT + 2, "Stab velocity");
	configOutput(ATMO_OUTPUT + 0, "Atmo V/OCT (poly 4)");
	configOutput(ATMO_OUTPUT + 1, "Atmo gate (poly 4)");
	configOutput(ATMO_OUTPUT + 2, "Atmo MOD (0..10 V, slow movement; MOVE knob)");
	configOutput(DUCK_OUTPUT, "Duck (10 V on every kick, falling: sidechain)");
	configOutput(RISE_OUTPUT, "Rise (0..10 V over a build)");
	songSeed = random::u32();
	generate(pat, settings(), random::u32());
	rebuildPlan();
	cur = plans[0].s[0];
	techno_registry::add(this);
	genralink::GenraInfo gi;
	gi.followPub = &followPub[0][0][0];
	gi.outputs = FOLLOW_OUTS;
	gi.stepPeriod = &stepPeriod;
	genralink::hub().addGenra(this, gi);
}

GenraTechno::~GenraTechno() {
	techno_registry::remove(this);
	genralink::hub().gone(this);
	delete incoming.exchange(nullptr);
	delete spent.exchange(nullptr);
	delete script.exchange(nullptr);
}

void GenraTechno::onRemove(const RemoveEvent& e) {
	techno_registry::remove(this);
	genralink::hub().gone(this);
}

void GenraTechno::onReset(const ResetEvent& e) {
	Module::onReset(e);
	style = 2;
	key = 9;
	forcedBuild = B_AUTO;
	for (int t = 0; t < NUM_TRACKS; t++) {
		typeSel[t] = -1;
		lock[t] = mute[t] = false;
	}
	Pattern p;
	generate(p, settings(), random::u32());
	post(p, true);
	rebuildPlan();
}

float GenraTechno::bpm() {
	if (inputs[CLK_INPUT].isConnected() && stepPeriod > 1.f)
		return 60.f * APP->engine->getSampleRate() / (stepPeriod * 4.f);
	return params[BPM_PARAM].getValue();
}

GenSettings GenraTechno::settings() {
	GenSettings g;
	g.style = style;
	g.key = key;
	g.energy = params[ENERGY_PARAM].getValue();
	for (int t = 0; t < NUM_TRACKS; t++) {
		g.dens[t] = params[DENS_PARAM + t].getValue();
		g.flavour[t] = params[FLAV_PARAM + t].getValue();
		g.len[t] = (int) params[LEN_PARAM + t].getValue();
		g.type[t] = typeSel[t];
	}
	return g;
}

void GenraTechno::post(const Pattern& p, bool now) {
	Pattern* q = new Pattern(p);
	incomingNow = now;
	delete incoming.exchange(q);
}

void GenraTechno::rebuildPlan() {
	std::vector<Section> s = planSong(style, params[LENGTH_PARAM].getValue(), params[BPM_PARAM].getValue(), songSeed,
	                                  forcedBuild, true, liveDj);
	int next = 1 - planCur.load();
	Plan& p = plans[next];
	p.n = (int) std::min<size_t>(s.size(), 96);
	for (int i = 0; i < p.n; i++)
		p.s[i] = s[i];
	planCur = next;
	updateView();
}

void GenraTechno::updateView() {
	std::lock_guard<std::mutex> lk(viewMx);
	view.sections.clear();
	SongScript* sc = script.load();
	if (sc) {
		view.sections = sc->rows;
		view.fromAblauf = true;
	}
	else {
		Plan& p = plans[planCur.load()];
		view.sections.assign(p.s, p.s + p.n);
		view.fromAblauf = false;
	}
}

// ---------------------------------------------------------------- the song (audio thread)
void GenraTechno::enterSection(const Section& s) {
	cur = s;
	secBar = 0;
	if (cur.part == P_DROP)
		dropsSoFar++;
	if (cur.part == P_BUILD)
		buildsSoFar++;
}

void GenraTechno::startSong(int fromRow) {
	songStep = 0;
	secBar = 0;
	inOv = false;
	dropsSoFar = buildsSoFar = 0;
	sinceTick = 0.f;
	phase = 1.f; // the first 16th at once
	SongScript* sc = script.load();
	if (sc && !sc->rows.empty()) {
		row = clamp(fromRow, 0, (int) sc->rows.size() - 1);
		const Section& s = sc->rows[row];
		if (s.pad < sc->pads.size())
			pat = sc->pads[s.pad];
		enterSection(s);
	}
	else {
		Plan& p = plans[planCur.load()];
		planIdx = 0;
		enterSection(p.n ? p.s[0] : Section());
	}
	wasSong = sc != nullptr;
	if (mainMode.load())
		enterMain();
	songStep = -1; // tick() adds one
	rstPulse.trigger(1e-3f);
}

void GenraTechno::enterMain() {
	// MAIN: the first drop of the song, 8 bars, again and again
	SongScript* sc = script.load();
	Section s;
	s.part = P_DROP;
	int idx = 0;
	if (sc && !sc->rows.empty()) {
		for (int i = (int) sc->rows.size() - 1; i >= 0; i--)
			if (sc->rows[i].part == P_DROP)
				idx = i;
		s = sc->rows[idx];
		row = idx;
		if (s.pad < sc->pads.size())
			pat = sc->pads[s.pad];
	}
	else {
		Plan& p = plans[planCur.load()];
		for (int i = p.n - 1; i >= 0; i--)
			if (p.s[i].part == P_DROP)
				idx = i;
		if (p.n)
			s = p.s[idx];
		planIdx = idx;
		inOv = false;
	}
	s.part = P_DROP;
	s.bars = 8;
	s.grow = 0;
	s.dropIdx = 0;
	mainIdx = idx;
	dropsSoFar = buildsSoFar = 0;
	enterSection(s);
	inMain = true;
}

void GenraTechno::jumpTo(int bar) {
	inMain = false;
	SongScript* sc = script.load();
	auto place = [&](const Section* secs, int n, int* idxOut) -> int {
		int total = 0;
		for (int i = 0; i < n; i++)
			total += secs[i].bars;
		int b = total > 0 ? bar % total : 0;
		int start = 0, r = 0;
		for (; r < n; r++) {
			if (b < start + secs[r].bars)
				break;
			start += secs[r].bars;
		}
		if (r >= n) {
			r = n - 1;
			start = total - secs[r].bars;
		}
		dropsSoFar = buildsSoFar = 0;
		for (int i = 0; i < r; i++) {
			dropsSoFar += secs[i].part == P_DROP;
			buildsSoFar += secs[i].part == P_BUILD;
		}
		*idxOut = r;
		return b - start;
	};
	if (sc && !sc->rows.empty()) {
		int r = 0;
		int off = place(sc->rows.data(), (int) sc->rows.size(), &r);
		row = r;
		const Section& s = sc->rows[r];
		if (s.pad < sc->pads.size())
			pat = sc->pads[s.pad];
		enterSection(s);
		secBar = clamp(off, 0, std::max(0, (int) s.bars - 1));
	}
	else {
		Plan& p = plans[planCur.load()];
		if (!p.n)
			return;
		int r = 0;
		int off = place(p.s, p.n, &r);
		planIdx = r;
		inOv = false;
		enterSection(p.s[r]);
		secBar = clamp(off, 0, std::max(0, (int) p.s[r].bars - 1));
	}
	wasSong = sc != nullptr;
}

void GenraTechno::advanceSection() {
	if (mainMode.load()) {
		enterMain();
		return;
	}
	inMain = false;
	SongScript* sc = script.load();
	if (sc && !wasSong)
		row = -1; // ABLAUF's song came in while LIVE played: from its first row
	wasSong = sc != nullptr;
	if (sc && !sc->rows.empty()) {
		int n = (int) sc->rows.size();
		if (sc->loopRow == row && row < n) {
			enterSection(sc->rows[row]);
			return;
		}
		row++;
		if (row >= n) {
			row = 0;
			dropsSoFar = buildsSoFar = 0;
		}
		const Section& s = sc->rows[row];
		if (s.pad < sc->pads.size())
			pat = sc->pads[s.pad];
		enterSection(s);
		return;
	}
	if (inOv && ovI + 1 < ovN) {
		ovI++;
		enterSection(ov[ovI]);
		return;
	}
	Plan& p = plans[planCur.load()];
	if (!inOv)
		planIdx++;
	inOv = false;
	if (planIdx >= p.n) {
		planIdx = 0;
		dropsSoFar = buildsSoFar = 0;
	}
	enterSection(p.n ? p.s[planIdx] : Section());
}

static Section mk(int part, int bars, int build = BT_NONE, int grow = 0, int dropIdx = 0) {
	Section s;
	s.part = (uint8_t) part;
	s.bars = (uint8_t) bars;
	s.build = (uint8_t) build;
	s.grow = (uint8_t) grow;
	s.dropIdx = (uint8_t) dropIdx;
	return s;
}

void GenraTechno::tick(float sr) {
	songStep++;
	int sib = (int) (((songStep % 16) + 16) % 16);
	if (sib == 0) {
		int jb = jumpBar.exchange(-1);
		if (jb >= 0) {
			mainMode = false;
			jumpTo(jb);
		}
		else if (mainReq.exchange(false) && mainMode.load())
			enterMain();
		else if (songStep > 0) {
			secBar++;
			// a part button held: the part goes on (8 bars more)
			if (secBar >= cur.bars && performHold.load() == cur.part && cur.bars <= 240)
				cur.bars += 8;
			if (secBar >= cur.bars || nextRowReq.exchange(false))
				advanceSection();
		}
		// PERFORM at the start of a phrase
		if (secBar % 8 == 0 && performReq.load() >= 0) {
			int req = performReq.exchange(-1);
			if (inMain) { // PERFORM leaves MAIN: the song goes on from the drop
				mainMode = false;
				inMain = false;
			}
			SongScript* sc = script.load();
			if (sc && !sc->rows.empty()) {
				int n = (int) sc->rows.size();
				for (int k = 1; k < n; k++) {
					int r = (row + k) % n;
					if (sc->rows[r].part == req) {
						row = r - 1;
						advanceSection();
						break;
					}
				}
			}
			else {
				int lastB = cur.build;
				int g = std::min(2, buildsSoFar);
				int b = forcedBuild != B_AUTO ? forcedBuild : pickBuild(style, hash(songSeed, (uint32_t) buildsSoFar, 9), lastB, g);
				int dg = std::min(2, dropsSoFar);
				ovN = 0;
				if (req == P_BUILD) {
					ov[ovN++] = mk(P_BUILD, g > 0 ? 16 : 8, b, g);
					ov[ovN++] = mk(P_DROP, 32, BT_NONE, std::min(2, dg), dropsSoFar);
				}
				else if (req == P_DROP)
					ov[ovN++] = mk(P_DROP, 32, BT_NONE, dg, dropsSoFar);
				else if (req == P_BREAK) {
					ov[ovN++] = mk(P_BREAK, 16);
					ov[ovN++] = mk(P_BUILD, g > 0 ? 16 : 8, b, g);
					ov[ovN++] = mk(P_DROP, 32, BT_NONE, std::min(2, dg + 1), dropsSoFar);
				}
				if (ovN) {
					if (!inOv)
						planIdx++; // afterwards: the section after the one we leave
					inOv = true;
					ovI = 0;
					enterSection(ov[0]);
				}
			}
		}
		// a new pattern at the start of a bar
		if (Pattern* q = incoming.exchange(nullptr)) {
			pat = *q;
			delete spent.exchange(q);
		}
		barPulse.trigger(1e-3f);
		pubBarCount++;
		int deg = pat.chordDeg[(songStep / 16) % std::max(1, (int) pat.bars) & 3];
		if (deg != lastChordDeg)
			pubChordCount++;
		lastChordDeg = deg;
		// EVOLVE: every 8 bars, maybe a small change of one track
		if (secBar % 8 == 0 && songStep > 0) {
			float ev = clamp(params[EVOLVE_PARAM].getValue() + inputs[EVOLVE_INPUT].getVoltage() / 10.f, 0.f, 1.f);
			if (random::uniform() < ev)
				mutateReq |= 1 << (int) (random::u32() % NUM_TRACKS);
		}
	}
	else if (incomingNow.load()) {
		if (Pattern* q = incoming.exchange(nullptr)) {
			pat = *q;
			delete spent.exchange(q);
		}
	}
	clkPulse.trigger(1e-3f);
	fireStep(sr);
	// what the displays show
	pubPart = cur.part;
	pubSecBar = secBar;
	pubSecBars = cur.bars;
	pubBuild = cur.build;
	pubDropIdx = cur.dropIdx;
	pubGrow = cur.grow;
	pubStepInBar = sib;
	pubSongStep = songStep;
	SongScript* sc = script.load();
	pubSongMode = sc != nullptr;
	if (sc && !sc->rows.empty()) {
		pubRow = row;
		pubSecIdx = row;
		int n = (int) sc->rows.size();
		pubNextPart = sc->loopRow == row ? (int) cur.part : (int) sc->rows[(row + 1) % n].part;
	}
	else {
		pubRow = -1;
		Plan& p = plans[planCur.load()];
		pubSecIdx = inOv ? planIdx - 1 : planIdx;
		if (inOv && ovI + 1 < ovN)
			pubNextPart = ov[ovI + 1].part;
		else
			pubNextPart = p.n ? p.s[(planIdx + 1) % p.n].part : P_GROOVE;
	}
	int bar = 0;
	if (sc && !sc->rows.empty()) {
		for (int i = 0; i < row && i < (int) sc->rows.size(); i++)
			bar += sc->rows[i].bars;
	}
	else {
		Plan& p = plans[planCur.load()];
		for (int i = 0; i < planIdx && i < p.n; i++)
			bar += p.s[i].bars;
	}
	pubSongBar = bar + secBar;
	pubMain = inMain;
	pubMainIdx = mainIdx;
	if (inMain) {
		pubSecIdx = mainIdx;
		pubNextPart = P_DROP;
	}
}

void GenraTechno::fireStep(float sr) {
	Ctx c;
	c.part = cur.part;
	c.secBar = secBar;
	c.secBars = std::max(1, (int) cur.bars);
	c.build = cur.build;
	c.grow = cur.grow;
	c.dropIdx = cur.dropIdx;
	c.nextPart = pubNextPart.load();
	SongScript* sc = script.load();
	c.djEdges = sc ? sc->djEdges : liveDj;
	int sib = (int) (songStep % 16);
	if (fillReq.exchange(false))
		fillBar = true;
	c.fill = fillBar;
	if (sib == 15)
		fillBar = false;
	c.energy = clamp(params[ENERGY_PARAM].getValue() + inputs[ENERGY_INPUT].getVoltage() / 10.f, 0.f, 1.f);
	c.songStep = songStep;
	c.rnd = songSeed;
	StepOut o;
	resolveStep(pat, c, o);
	pubSecPos = (secBar * 16 + sib) / (float) (c.secBars * 16);
	pubRise = o.rise;
	int so = solo.load();
	auto silent = [&](int t) { return mute[t] || (so >= 0 && so != t); };
	float swing = (params[SWING_PARAM].getValue() - 50.f) / 50.f;
	float base = (sib & 1) ? swing * stepPeriod : 0.f;
	if (!silent(T_DRUMS))
		for (int v = 0; v < 8; v++) {
			const DrumFire& f = o.drum[v];
			if (!f.vel)
				continue;
			DrumV& d = drums[v];
			d.delay = std::max(0.f, base + f.micro * stepPeriod / 100.f);
			d.pVel = f.vel / 127.f;
			d.pRat = std::max(1, (int) f.ratch);
		}
	for (int t = 0; t < MELS; t++) {
		const NoteFire& f = o.mel[t];
		if (!f.on || silent(t + 1))
			continue;
		MelV& m = mels[t];
		m.delay = std::max(0.f, base + f.note.micro * stepPeriod / 100.f);
		m.pending = f.note;
		m.pendingOct = f.octave;
	}
}

void GenraTechno::process(const ProcessArgs& args) {
	float sr = args.sampleRate;
	// ---- transport
	if (runTrig.process(inputs[RUN_INPUT].getVoltage(), 0.1f, 1.f)) {
		if (running)
			stopReq = true;
		else
			runReq = true;
	}
	if (rstTrig.process(inputs[RST_INPUT].getVoltage(), 0.1f, 1.f) && running)
		startRow = 0;
	if (newTrig.process(inputs[NEW_INPUT].getVoltage(), 0.1f, 1.f))
		newReq |= 1 << 5;
	if (buildTrig.process(inputs[BUILD_INPUT].getVoltage(), 0.1f, 1.f))
		performReq = P_BUILD;
	if (dropTrig.process(inputs[DROP_INPUT].getVoltage(), 0.1f, 1.f))
		performReq = P_DROP;
	int sr0 = startRow.exchange(-1);
	if (sr0 >= 0) {
		startSong(sr0);
		running = true;
	}
	if (runReq.exchange(false)) {
		startSong(0);
		running = true;
	}
	if (stopReq.exchange(false)) {
		running = false;
		for (int t = 0; t < MELS; t++) {
			mels[t].gate = 0.f;
			mels[t].delay = -1.f;
			mels[t].ratLeft = 0;
		}
		for (int v = 0; v < 8; v++) {
			drums[v].delay = -1.f;
			drums[v].ratLeft = 0;
		}
	}
	// a pattern from the UI while stopped: at once
	if (!running && incoming.load()) {
		if (Pattern* q = incoming.exchange(nullptr)) {
			pat = *q;
			delete spent.exchange(q);
		}
	}
	// ---- clock
	bool ext = inputs[CLK_INPUT].isConnected();
	sinceClock += 1.f;
	bool doTick = false;
	if (ext) {
		if (clkTrig.process(inputs[CLK_INPUT].getVoltage(), 0.1f, 1.f)) {
			if (extClockSeen && sinceClock > 10.f)
				stepPeriod = 0.7f * stepPeriod + 0.3f * sinceClock;
			extClockSeen = true;
			sinceClock = 0.f;
			doTick = running;
		}
	}
	else {
		extClockSeen = false;
		stepPeriod = sr * 60.f / std::max(20.f, params[BPM_PARAM].getValue()) / 4.f;
		if (running) {
			phase += 1.f / stepPeriod;
			if (phase >= 1.f) {
				phase -= std::floor(phase);
				doTick = true;
			}
		}
	}
	if (doTick)
		tick(sr);
	// ---- drums
	for (int v = 0; v < 8; v++) {
		DrumV& d = drums[v];
		if (d.delay >= 0.f) {
			d.delay -= 1.f;
			if (d.delay < 0.f) {
				d.trig = 0.005f * sr;
				d.vel = d.pVel;
				d.ratLeft = d.pRat - 1;
				d.ratInterval = stepPeriod / d.pRat;
				d.ratCounter = d.ratInterval;
				if (v == D_KICK)
					duck = 1.f;
			}
		}
		if (d.ratLeft > 0) {
			d.ratCounter -= 1.f;
			if (d.ratCounter <= 0.f) {
				d.ratLeft--;
				d.ratCounter += d.ratInterval;
				d.trig = std::min(0.005f * sr, d.ratInterval * 0.5f);
			}
		}
		float out = 0.f;
		if (d.trig > 0.f) {
			d.trig -= 1.f;
			out = 10.f * d.vel;
		}
		outputs[DRUM_OUTPUT + v].setVoltage(out);
	}
	// ---- the melodic voices
	for (int t = 0; t < MELS; t++) {
		MelV& m = mels[t];
		if (m.delay >= 0.f) {
			m.delay -= 1.f;
			if (m.delay < 0.f) {
				const Note& n = m.pending;
				bool legato = m.slide && m.gate > 0.f;
				if (m.gate > 0.f && !legato)
					m.retrig = 0.001f * sr;
				m.n = std::max(1, std::min(4, (int) n.n));
				for (int i = 0; i < 4; i++) {
					int pitch = n.pitch[std::min(i, m.n - 1)] + 12 * m.pendingOct;
					m.voct[i] = (pitch - 60) / 12.f;
				}
				m.vel = n.vel / 127.f;
				m.acc = n.acc;
				m.slide = n.slide;
				float len = std::max(1, (int) n.len) * stepPeriod;
				int r = std::max(1, (int) n.ratch);
				if (r > 1) {
					m.ratLeft = r - 1;
					m.ratInterval = len / r;
					m.ratCounter = m.ratInterval;
					m.ratGate = m.ratInterval * 0.5f;
					m.gate = m.ratGate;
				}
				else {
					m.ratLeft = 0;
					m.gate = n.slide ? len + 0.004f * sr : std::max(0.002f * sr, len - 0.003f * sr);
				}
			}
		}
		if (m.retrig > 0.f)
			m.retrig -= 1.f;
		if (m.gate > 0.f)
			m.gate -= 1.f;
		if (m.ratLeft > 0) {
			m.ratCounter -= 1.f;
			if (m.ratCounter <= 0.f) {
				m.ratLeft--;
				m.ratCounter += m.ratInterval;
				m.retrig = 0.001f * sr;
				m.gate = m.ratGate;
			}
		}
	}
	auto gateOn = [&](const MelV& m) { return m.gate > 0.f && m.retrig <= 0.f; };
	auto mono = [&](int out, const MelV& m) {
		outputs[out + 0].setVoltage(m.voct[0]);
		outputs[out + 1].setVoltage(gateOn(m) ? 10.f : 0.f);
		outputs[out + 2].setVoltage(m.vel * 10.f);
		outputs[out + 3].setVoltage(m.acc ? 10.f : 0.f);
		outputs[out + 4].setVoltage(m.slide ? 10.f : 0.f);
	};
	mono(BASS_OUTPUT, mels[0]);
	mono(LEAD_OUTPUT, mels[2]);
	auto poly = [&](int voct, int gate, const MelV& m) {
		outputs[voct].setChannels(4);
		outputs[gate].setChannels(4);
		bool g = gateOn(m);
		for (int i = 0; i < 4; i++) {
			outputs[voct].setVoltage(m.voct[i], i);
			outputs[gate].setVoltage(g && i < m.n ? 10.f : 0.f, i);
		}
	};
	poly(STAB_OUTPUT + 0, STAB_OUTPUT + 1, mels[1]);
	outputs[STAB_OUTPUT + 2].setVoltage(mels[1].vel * 10.f);
	poly(ATMO_OUTPUT + 0, ATMO_OUTPUT + 1, mels[3]);
	{
		// MOD: a slow movement over 8 bars, its depth from MOVE
		double pos = songStep + std::min(0.999f, sinceTick / std::max(1.f, stepPeriod));
		float lfo = 0.5f + 0.5f * std::sin(2.f * (float) M_PI * (float) (pos / 128.0));
		outputs[ATMO_OUTPUT + 2].setVoltage(10.f * lfo * params[FLAV_PARAM + T_ATMO].getValue());
	}
	sinceTick = doTick ? 0.f : sinceTick + 1.f;
	// ---- clock outputs, duck, rise
	outputs[CLK_OUTPUT].setVoltage(clkPulse.process(args.sampleTime) ? 10.f : 0.f);
	outputs[RST_OUTPUT].setVoltage(rstPulse.process(args.sampleTime) ? 10.f : 0.f);
	outputs[BAR_OUTPUT].setVoltage(barPulse.process(args.sampleTime) ? 10.f : 0.f);
	duck *= 1.f - 1.f / (0.15f * sr);
	outputs[DUCK_OUTPUT].setVoltage(10.f * duck);
	float rise = running ? pubRise.load() : 0.f;
	riseSm += (rise - riseSm) * 0.0005f;
	outputs[RISE_OUTPUT].setVoltage(10.f * riseSm);
	publishFollow(args.frame);
}

void GenraTechno::publishFollow(int64_t frame) {
	float(*p)[genraports::PUB_CHANNELS] = followPub[frame & 1];
	auto put = [&](int to, int from) {
		for (int c = 0; c < genraports::PUB_CHANNELS; c++)
			p[to][c] = outputs[from].getVoltage(c);
	};
	for (int v = 0; v < 8; v++)
		put(genraports::DRUM_OUTPUT + v, DRUM_OUTPUT + v); // the same order as GENRA
	put(genraports::BASS_VOCT_OUTPUT, BASS_OUTPUT + 0);
	put(genraports::BASS_GATE_OUTPUT, BASS_OUTPUT + 1);
	put(genraports::BASS_VEL_OUTPUT, BASS_OUTPUT + 2);
	put(genraports::BASS_ACC_OUTPUT, BASS_OUTPUT + 3);
	put(genraports::BASS_SLIDE_OUTPUT, BASS_OUTPUT + 4);
	put(genraports::MEL_VOCT_OUTPUT, LEAD_OUTPUT + 0);
	put(genraports::MEL_GATE_OUTPUT, LEAD_OUTPUT + 1);
	put(genraports::MEL_VEL_OUTPUT, LEAD_OUTPUT + 2);
	put(genraports::MEL_ACC_OUTPUT, LEAD_OUTPUT + 3);
	put(genraports::MEL_SLIDE_OUTPUT, LEAD_OUTPUT + 4);
	put(genraports::PAD_VOCT_OUTPUT, ATMO_OUTPUT + 0);
	put(genraports::PAD_GATE_OUTPUT, ATMO_OUTPUT + 1);
	for (int c = 0; c < genraports::PUB_CHANNELS; c++)
		p[genraports::PAD_VEL_OUTPUT][c] = 8.f;
	put(genraports::PAD_MOD_OUTPUT, ATMO_OUTPUT + 2);
	put(genraports::CLOCK_OUTPUT, CLK_OUTPUT);
	put(genraports::RESET_OUTPUT, RST_OUTPUT);
}

json_t* GenraTechno::dataToJson() {
	json_t* root = json_object();
	json_object_set_new(root, "version", json_integer(1));
	json_object_set_new(root, "pattern", json_string(string::toBase64((const uint8_t*) &pat, sizeof(Pattern)).c_str()));
	json_object_set_new(root, "style", json_integer(style));
	json_object_set_new(root, "key", json_integer(key));
	json_object_set_new(root, "build", json_integer(forcedBuild));
	json_object_set_new(root, "seed", json_integer(songSeed));
	json_object_set_new(root, "main", json_boolean(mainMode.load()));
	json_object_set_new(root, "liveDj", json_boolean(liveDj));
	json_t* tr = json_array();
	for (int t = 0; t < NUM_TRACKS; t++) {
		json_t* o = json_object();
		json_object_set_new(o, "type", json_integer(typeSel[t]));
		json_object_set_new(o, "lock", json_boolean(lock[t]));
		json_object_set_new(o, "mute", json_boolean(mute[t]));
		json_array_append_new(tr, o);
	}
	json_object_set_new(root, "tracks", tr);
	return root;
}

void GenraTechno::dataFromJson(json_t* root) {
	if (json_t* j = json_object_get(root, "style"))
		style = clamp((int) json_integer_value(j), 0, numStyles() - 1);
	if (json_t* j = json_object_get(root, "key"))
		key = clamp((int) json_integer_value(j), 0, 11);
	if (json_t* j = json_object_get(root, "build"))
		forcedBuild = clamp((int) json_integer_value(j), 0, NUM_BUILDS - 1);
	if (json_t* j = json_object_get(root, "seed"))
		songSeed = (uint32_t) json_integer_value(j);
	mainMode = !json_is_false(json_object_get(root, "main"));
	liveDj = json_is_true(json_object_get(root, "liveDj"));
	if (json_t* tr = json_object_get(root, "tracks"))
		for (int t = 0; t < NUM_TRACKS && t < (int) json_array_size(tr); t++) {
			json_t* o = json_array_get(tr, t);
			if (json_t* j = json_object_get(o, "type"))
				typeSel[t] = clamp((int) json_integer_value(j), -1, typeCount(t) - 1);
			lock[t] = json_is_true(json_object_get(o, "lock"));
			mute[t] = json_is_true(json_object_get(o, "mute"));
		}
	if (json_t* j = json_object_get(root, "pattern")) {
		std::vector<uint8_t> b = string::fromBase64(json_string_value(j));
		if (b.size() == sizeof(Pattern)) {
			Pattern p;
			std::memcpy((void*) &p, b.data(), sizeof(Pattern));
			post(p, true);
			pat = p;
		}
	}
	rebuildPlan();
}

// ================================================================ the panel
struct GenraTechnoWidget : ModuleWidget {
	GenraTechno* gm = nullptr;
	std::deque<Pattern> undo;
	float lastLen[NUM_TRACKS] = {-1, -1, -1, -1, -1};
	float lastLength = -1.f, lastBpm = -1.f;
	std::string flash;
	float flashT = 0.f;
	float lastParam[GenraTechno::NUM_PARAMS] = {};
	bool paramsSeen = false;

	Rect dispBox;
	Vec hover = Vec(-1, -1);
	// the timeline in the display (px, relative to the display)
	float tlX0() { return 10.f; }
	float tlX1() { return dispBox.size.x - 52.f; }
	bool inBand(Vec p) { return p.y > 38.f && p.y < 60.f && p.x >= tlX0() && p.x <= tlX1(); }
	std::vector<Section> sections() {
		std::vector<Section> s;
		if (gm) {
			std::lock_guard<std::mutex> lk(gm->viewMx);
			s = gm->view.sections;
		}
		return s;
	}
	/** The section under x and the bar a click there jumps to (start of the 8-bar phrase). */
	int barAt(float x, int* secOut) {
		std::vector<Section> s = sections();
		int total = songBars(s);
		if (total <= 0)
			return -1;
		float f = clamp((x - tlX0()) / (tlX1() - tlX0()), 0.f, 0.9999f);
		int bar = (int) (f * total), start = 0;
		for (size_t i = 0; i < s.size(); i++) {
			if (bar < start + s[i].bars) {
				if (secOut)
					*secOut = (int) i;
				return start + ((bar - start) / 8) * 8;
			}
			start += s[i].bars;
		}
		return 0;
	}
	void jumpAt(float x) {
		if (!gm)
			return;
		int b = barAt(x, nullptr);
		if (b < 0)
			return;
		gm->jumpBar = b;
		if (!gm->running.load())
			gm->runReq = true;
	}
	void partStep(int d) {
		if (!gm)
			return;
		std::vector<Section> s = sections();
		if (s.empty())
			return;
		bool run = gm->running.load();
		int cur = gm->pubMain.load() ? gm->pubMainIdx.load() : (run ? gm->pubSecIdx.load() : 0);
		if (!run && d > 0 && !gm->mainMode.load())
			cur = -1;
		int t = clamp(cur + d, 0, (int) s.size() - 1);
		int start = 0;
		for (int i = 0; i < t; i++)
			start += s[i].bars;
		gm->jumpBar = start;
		if (!run)
			gm->runReq = true;
	}

	void pushUndo() {
		if (!gm)
			return;
		undo.push_back(gm->pat);
		while (undo.size() > 24)
			undo.pop_front();
	}

	/** New pattern (track -1: all unlocked tracks). */
	void regenerate(int track) {
		if (!gm)
			return;
		pushUndo();
		Pattern p = gm->pat;
		GenSettings g = gm->settings();
		uint32_t seed = random::u32();
		bool anyLock = false;
		for (int t = 0; t < NUM_TRACKS; t++)
			anyLock = anyLock || gm->lock[t];
		if (track < 0 && !anyLock)
			generate(p, g, seed, -1);
		else if (track < 0) {
			for (int t = 0; t < NUM_TRACKS; t++)
				if (!gm->lock[t])
					generate(p, g, hash(seed, t), t);
		}
		else
			generate(p, g, seed, track);
		gm->post(p, !gm->running.load());
	}
	void mutateTracks(int mask, float amount) {
		if (!gm)
			return;
		Pattern p = gm->pat;
		GenSettings g = gm->settings();
		bool any = false;
		for (int t = 0; t < NUM_TRACKS; t++)
			if ((mask >> t) & 1 && !gm->lock[t]) {
				mutate(p, g, random::u32(), t, amount);
				any = true;
			}
		if (any)
			gm->post(p, false);
	}
	void setStyle(int s) {
		if (!gm || s == gm->style)
			return;
		gm->style = s;
		const Style& st = styles()[s];
		gm->params[GenraTechno::BPM_PARAM].setValue(st.bpm);
		gm->params[GenraTechno::SWING_PARAM].setValue(st.swing);
		regenerate(-1);
		gm->rebuildPlan();
	}
	void setKey(int k) {
		if (!gm || k == gm->key)
			return;
		pushUndo();
		int d = ((k - gm->key) % 12 + 12) % 12;
		if (d > 5)
			d -= 12;
		Pattern p = gm->pat;
		for (int t = 0; t < MELS; t++)
			for (int i = 0; i < STEPS; i++)
				for (int v = 0; v < VOICES; v++)
					if (p.mel[t][i].on && v < p.mel[t][i].n)
						p.mel[t][i].pitch[v] = (int8_t) clamp(p.mel[t][i].pitch[v] + d, 0, 127);
		for (int b = 0; b < 4; b++)
			for (int v = 0; v < VOICES; v++)
				if (v < p.chordN[b])
					p.chord[b][v] = (int8_t) (p.chord[b][v] + d);
		p.key = (uint8_t) k;
		gm->key = k;
		gm->post(p, true);
	}

	GenraTechnoWidget(GenraTechno* module) {
		setModule(module);
		gm = module;
		setPanel(createPanel(asset::plugin(pluginInstance, "res/GenraTechno.svg")));
		using namespace tui;
		// knobs
		addParam(createParamCentered<GKnob>(mm(L::K_BPM), module, GenraTechno::BPM_PARAM));
		addParam(createParamCentered<GKnob>(mm(L::K_SWING), module, GenraTechno::SWING_PARAM));
		addParam(createParamCentered<GLargeKnob>(mm(L::K_ENERGY), module, GenraTechno::ENERGY_PARAM));
		addParam(createParamCentered<GLargeKnob>(mm(L::K_EVOLVE), module, GenraTechno::EVOLVE_PARAM));
		addParam(createParamCentered<GKnob>(mm(L::K_LENGTH), module, GenraTechno::LENGTH_PARAM));
		static const float* const KD[5] = {L::K_DENS0, L::K_DENS1, L::K_DENS2, L::K_DENS3, L::K_DENS4};
		static const float* const KF[5] = {L::K_FLAV0, L::K_FLAV1, L::K_FLAV2, L::K_FLAV3, L::K_FLAV4};
		static const float* const KL[5] = {L::K_LEN0, L::K_LEN1, L::K_LEN2, L::K_LEN3, L::K_LEN4};
		for (int t = 0; t < NUM_TRACKS; t++) {
			addParam(createParamCentered<GLargeKnob>(mm(KD[t]), module, GenraTechno::DENS_PARAM + t));
			addParam(createParamCentered<GKnob>(mm(KF[t]), module, GenraTechno::FLAV_PARAM + t));
			addParam(createParamCentered<GSmallKnob>(mm(KL[t]), module, GenraTechno::LEN_PARAM + t));
		}
		// jacks
		static const float* const IN[8] = {L::IN_CLK, L::IN_RST, L::IN_RUN, L::IN_NEW, L::IN_BUILD, L::IN_DROP, L::IN_ENERGY, L::IN_EVOLVE};
		for (int i = 0; i < 8; i++)
			addInput(createInputCentered<PJ301MPort>(mm(IN[i]), module, i));
		static const float* const DR[8] = {L::OUT_DRUM0, L::OUT_DRUM1, L::OUT_DRUM2, L::OUT_DRUM3, L::OUT_DRUM4, L::OUT_DRUM5, L::OUT_DRUM6, L::OUT_DRUM7};
		for (int v = 0; v < 8; v++)
			addOutput(createOutputCentered<PJ301MPort>(mm(DR[v]), module, GenraTechno::DRUM_OUTPUT + v));
		addOutput(createOutputCentered<PJ301MPort>(mm(L::OUT_CLK), module, GenraTechno::CLK_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm(L::OUT_RST), module, GenraTechno::RST_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm(L::OUT_BAR), module, GenraTechno::BAR_OUTPUT));
		static const float* const BO[5] = {L::OUT_BASS0, L::OUT_BASS1, L::OUT_BASS2, L::OUT_BASS3, L::OUT_BASS4};
		static const float* const LO[5] = {L::OUT_LEAD0, L::OUT_LEAD1, L::OUT_LEAD2, L::OUT_LEAD3, L::OUT_LEAD4};
		for (int i = 0; i < 5; i++) {
			addOutput(createOutputCentered<PJ301MPort>(mm(BO[i]), module, GenraTechno::BASS_OUTPUT + i));
			addOutput(createOutputCentered<PJ301MPort>(mm(LO[i]), module, GenraTechno::LEAD_OUTPUT + i));
		}
		static const float* const SO[3] = {L::OUT_STAB0, L::OUT_STAB1, L::OUT_STAB2};
		static const float* const AO[3] = {L::OUT_ATMO0, L::OUT_ATMO1, L::OUT_ATMO2};
		for (int i = 0; i < 3; i++) {
			addOutput(createOutputCentered<PJ301MPort>(mm(SO[i]), module, GenraTechno::STAB_OUTPUT + i));
			addOutput(createOutputCentered<PJ301MPort>(mm(AO[i]), module, GenraTechno::ATMO_OUTPUT + i));
		}
		addOutput(createOutputCentered<PJ301MPort>(mm(L::OUT_DUCK), module, GenraTechno::DUCK_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm(L::OUT_RISE), module, GenraTechno::RISE_OUTPUT));

		// transport
		TButton* play = place<TButton>(this, L::BTN_PLAY);
		play->label = "PLAY";
		play->col = RED;
		play->textCol = WHITE;
		play->lit = [=]() { return gm && gm->running.load(); };
		play->act = [=]() {
			if (gm)
				gm->runReq = true;
		};
		TButton* stop = place<TButton>(this, L::BTN_STOP);
		stop->label = "STOP";
		stop->act = [=]() {
			if (gm)
				gm->stopReq = true;
		};
		// PERFORM
		static const float* const PB[4] = {L::BTN_BUILD, L::BTN_DROP, L::BTN_BREAK, L::BTN_FILL};
		static const char* const PN[4] = {"BUILD", "DROP", "BREAK", "FILL"};
		static const int PP[4] = {P_BUILD, P_DROP, P_BREAK, -1};
		for (int i = 0; i < 4; i++) {
			TButton* b = place<TButton>(this, PB[i]);
			b->label = PN[i];
			b->col = i == 1 ? RED : nvgRGB(0x3a, 0x2a, 0x2e);
			b->textCol = i == 1 ? WHITE : nvgRGB(0xff, 0xb3, 0xb3);
			b->fontSize = 10.f;
			int part = PP[i];
			b->lit = [=]() { return gm && part >= 0 && gm->performReq.load() == part; };
			b->act = [=]() {
				if (!gm)
					return;
				if (part < 0)
					gm->fillReq = true;
				else {
					gm->performReq = part;
					gm->performHold = part;
				}
			};
			b->release = [=]() {
				if (gm)
					gm->performHold = -1;
			};
		}
		TButton* na = place<TButton>(this, L::BTN_NEWALL);
		na->label = "NEW ALL";
		na->act = [=]() {
			if (gm) {
				gm->songSeed = random::u32();
				regenerate(-1);
				gm->rebuildPlan();
				gm->mainMode = true; // a new theme: hear it in MAIN first
				if (gm->running.load())
					gm->mainReq = true;
			}
		};
		TButton* mu = place<TButton>(this, L::BTN_MUTATE);
		mu->label = "MUTATE";
		mu->act = [=]() {
			pushUndo();
			mutateTracks(31, 0.6f);
		};
		TButton* la = place<TButton>(this, L::BTN_LOCKALL);
		la->label = "LOCK";
		la->lit = [=]() {
			if (!gm)
				return false;
			for (int t = 0; t < NUM_TRACKS; t++)
				if (!gm->lock[t])
					return false;
			return true;
		};
		la->act = [=]() {
			if (!gm)
				return;
			bool all = true;
			for (int t = 0; t < NUM_TRACKS; t++)
				all = all && gm->lock[t];
			for (int t = 0; t < NUM_TRACKS; t++)
				gm->lock[t] = !all;
		};
		TButton* un = place<TButton>(this, L::BTN_UNDO);
		un->label = "UNDO";
		un->act = [=]() {
			if (gm && !undo.empty()) {
				gm->post(undo.back(), true);
				undo.pop_back();
			}
		};
		// fields
		TField* fam = place<TField>(this, L::F_FAMILY);
		fam->prefix = "FAMILY";
		fam->value = [=]() { return gm ? std::string(FAMILY_NAMES[styles()[gm->style].family]) : std::string("BERLIN"); };
		fam->count = []() { return (int) NUM_FAMILIES; };
		fam->current = [=]() { return gm ? styles()[gm->style].family : 1; };
		fam->item = [](int i) { return std::string(FAMILY_NAMES[i]); };
		fam->choose = [=](int i) {
			std::vector<int> s = stylesOf(i);
			if (!s.empty())
				setStyle(s[0]);
		};
		TField* sty = place<TField>(this, L::F_STYLE);
		sty->prefix = "STYLE";
		sty->value = [=]() { return std::string(styles()[gm ? gm->style : 2].name); };
		sty->count = []() { return numStyles(); };
		sty->current = [=]() { return gm ? gm->style : 2; };
		sty->item = [](int i) { return std::string(FAMILY_NAMES[styles()[i].family]) + " › " + styles()[i].name; };
		sty->separatorBefore = [](int i) { return i > 0 && styles()[i].family != styles()[i - 1].family; };
		sty->choose = [=](int i) { setStyle(i); };
		TField* ky = place<TField>(this, L::F_KEY);
		ky->prefix = "KEY";
		ky->value = [=]() {
			if (!gm)
				return std::string("A MINOR");
			return std::string(KEY_NAMES[gm->key]) + " " + SCALES[styles()[gm->style].mode].name;
		};
		ky->count = []() { return 12; };
		ky->current = [=]() { return gm ? gm->key : 9; };
		ky->item = [](int i) { return std::string(KEY_NAMES[i]); };
		ky->choose = [=](int i) { setKey(i); };
		TField* bu = place<TField>(this, L::F_BUILD);
		bu->prefix = "BUILD";
		bu->col = YEL;
		bu->value = [=]() { return std::string(BUILD_NAMES[gm ? gm->forcedBuild : 0]); };
		bu->count = []() { return (int) NUM_BUILDS; };
		bu->current = [=]() { return gm ? gm->forcedBuild : 0; };
		bu->item = [](int i) { return std::string(BUILD_NAMES[i]); };
		bu->choose = [=](int i) {
			if (gm) {
				gm->forcedBuild = i;
				gm->rebuildPlan();
			}
		};
		// the tracks
		static const float* const FT[5] = {L::F_TYPE0, L::F_TYPE1, L::F_TYPE2, L::F_TYPE3, L::F_TYPE4};
		static const float* const BN[5] = {L::BTN_NEW0, L::BTN_NEW1, L::BTN_NEW2, L::BTN_NEW3, L::BTN_NEW4};
		static const float* const BL[5] = {L::BTN_LOCK0, L::BTN_LOCK1, L::BTN_LOCK2, L::BTN_LOCK3, L::BTN_LOCK4};
		static const float* const BM[5] = {L::BTN_MUTE0, L::BTN_MUTE1, L::BTN_MUTE2, L::BTN_MUTE3, L::BTN_MUTE4};
		for (int t = 0; t < NUM_TRACKS; t++) {
			TField* f = place<TField>(this, FT[t]);
			f->col = TRACK_COL[t];
			f->value = [=]() {
				if (!gm)
					return std::string(typeName(t, styleType(2, t)));
				std::string s = typeName(t, gm->effectiveType(t));
				return gm->typeSel[t] < 0 ? s : s + " *";
			};
			f->count = [=]() { return typeCount(t) + 1; };
			f->current = [=]() { return gm ? gm->typeSel[t] + 1 : 0; };
			f->item = [=](int i) {
				if (i == 0)
					return std::string("AUTO (the style's: ") + typeName(t, styleType(gm ? gm->style : 2, t)) + ")";
				return std::string(typeName(t, i - 1));
			};
			f->choose = [=](int i) {
				if (!gm)
					return;
				gm->typeSel[t] = i - 1;
				regenerate(t);
			};
			TButton* n = place<TButton>(this, BN[t]);
			n->label = "NEW";
			n->act = [=]() { regenerate(t); };
			TButton* l = place<TButton>(this, BL[t]);
			l->label = "LOCK";
			l->lit = [=]() { return gm && gm->lock[t]; };
			l->act = [=]() {
				if (gm)
					gm->lock[t] = !gm->lock[t];
			};
			TButton* m = place<TButton>(this, BM[t]);
			m->label = "MUTE";
			m->lit = [=]() { return gm && gm->mute[t]; };
			m->colOf = [=]() { return gm && gm->mute[t] ? dark(TRACK_COL[t], 0.6f) : BTN; };
			m->act = [=]() {
				if (gm)
					gm->mute[t] = !gm->mute[t];
			};
		}
		// the display, the timeline in it (click: jump there at the end of the bar) and its keys
		Drawn* d = place<Drawn>(this, L::DISP_MAIN);
		d->paintLit = [=](const DrawArgs& a) { drawDisplay(a.vg, d->box.size); };
		dispBox = d->box;
		struct Timeline : widget::OpaqueWidget {
			GenraTechnoWidget* w = nullptr;
			void onButton(const ButtonEvent& e) override {
				if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT && w->inBand(e.pos)) {
					w->jumpAt(e.pos.x);
					e.consume(this);
				}
			}
			void onHover(const HoverEvent& e) override {
				w->hover = e.pos;
				OpaqueWidget::onHover(e);
			}
			void onLeave(const LeaveEvent& e) override { w->hover = Vec(-1, -1); }
		};
		Timeline* tl = new Timeline;
		tl->w = this;
		tl->box = d->box;
		addChild(tl);
		TButton* pv = place<TButton>(this, L::BTN_PREV);
		pv->label = "< PART";
		pv->fontSize = 7.5f;
		pv->act = [=]() { partStep(-1); };
		TButton* nx = place<TButton>(this, L::BTN_NEXT);
		nx->label = "PART >";
		nx->fontSize = 7.5f;
		nx->act = [=]() { partStep(1); };
		TButton* mn = place<TButton>(this, L::BTN_MAIN);
		mn->label = "MAIN";
		mn->fontSize = 8.f;
		mn->colOf = [=]() { return gm && gm->mainMode.load() ? RED : BTN; };
		mn->textCol = WHITE;
		mn->act = [=]() {
			if (!gm)
				return;
			gm->mainMode = true;
			if (gm->running.load())
				gm->mainReq = true;
		};
		TButton* sg = place<TButton>(this, L::BTN_SONGMODE);
		sg->labelOf = [=]() { return std::string(gm && gm->pubSongMode.load() ? "ABLAUF SONG" : "LIVE SONG"); };
		sg->fontSize = 7.f;
		sg->colOf = [=]() { return gm && !gm->mainMode.load() ? dark(PART_COL[1], 0.9f) : BTN; };
		sg->act = [=]() {
			if (!gm || !gm->mainMode.load())
				return;
			gm->mainMode = false;
			gm->jumpBar = 0;
			if (!gm->running.load())
				gm->runReq = true;
		};
	}

	void drawDisplay(NVGcontext* vg, Vec sz) {
		using namespace tui;
		auto f = bold();
		float W = sz.x, H = sz.y;
		int st = gm ? gm->style : 2;
		const Style& s = styles()[st];
		textFit(vg, f, 10, 17, std::string(FAMILY_NAMES[s.family]) + "  ›  " + s.name, 15.f, mm2px(63.f), TXT, NVG_ALIGN_LEFT, 1.f);
		float bpm = gm ? gm->bpm() : s.bpm;
		float en = gm ? gm->params[GenraTechno::ENERGY_PARAM].getValue() : 0.6f;
		float sw = gm ? gm->params[GenraTechno::SWING_PARAM].getValue() : s.swing;
		std::string info = string::f("%.0f BPM    %s %s    SWING %.0f%%    ENERGY %d/10", bpm, KEY_NAMES[gm ? gm->key : 9],
		                             SCALES[s.mode].name, sw, (int) std::round(en * 10));
		if (flashT > 0.f)
			text(vg, f, 10, 33, flash, 9.5f, YEL, NVG_ALIGN_LEFT, 0.8f);
		else
			text(vg, f, 10, 33, info, 8.5f, DIM, NVG_ALIGN_LEFT, 0.8f);
		// energy meter
		for (int i = 0; i < 10; i++) {
			NVGcolor c = i >= 7 ? RED : (i >= 4 ? YEL : TRACK_COL[1]);
			if (i >= (int) std::round(en * 10))
				c = dark(c, 0.25f);
			roundRect(vg, W - 40, 40 - i * 3.6f, 14, 2.4f, 0.6f, c);
		}
		// the song: sections in their colours over the whole width, where we are
		std::vector<Section> secs;
		if (gm) {
			std::lock_guard<std::mutex> lk(gm->viewMx);
			secs = gm->view.sections;
		}
		float x0 = 10, x1 = W - 52, y = 44, h = 13;
		int total = std::max(1, songBars(secs));
		float x = x0;
		int curIdx = gm ? gm->pubSecIdx.load() : 0;
		bool inMain = gm && gm->running.load() ? gm->pubMain.load() : (gm && gm->mainMode.load());
		int mainIdx = 0;
		for (int i = (int) secs.size() - 1; i >= 0; i--)
			if (secs[i].part == P_DROP)
				mainIdx = i;
		if (inMain)
			curIdx = mainIdx;
		int hoverSec = -1;
		if (hover.x >= 0 && inBand(hover))
			barAt(hover.x, &hoverSec);
		for (size_t i = 0; i < secs.size(); i++) {
			float w = (x1 - x0) * secs[i].bars / total;
			bool now = (int) i == curIdx && gm && (gm->running.load() || inMain);
			NVGcolor c = PART_COL[secs[i].part % 6];
			roundRect(vg, x + 0.5f, y, std::max(1.f, w - 1.f), h, 1.2f, now ? c : dark(c, inMain ? 0.3f : 0.5f));
			if ((int) i == hoverSec)
				frame(vg, x + 0.5f, y, std::max(1.f, w - 1.f), h, 1.2f, WHITE, 1.2f);
			if (w > 26)
				textFit(vg, f, x + w / 2, y + h / 2 + 0.5f, PART_NAMES[secs[i].part % 6], 7.f, w - 4, now ? BG : (inMain ? DIM : TXT),
				        NVG_ALIGN_CENTER, 0.6f);
			if (inMain && (int) i == mainIdx)
				text(vg, f, x + w / 2, y - 6, "LOOP 8 BARS · ALL TRACKS", 6.5f, WHITE, NVG_ALIGN_CENTER, 0.5f);
			if ((int) i == hoverSec && !(inMain && (int) i == mainIdx)) {
				std::string lab = std::string("JUMP › ") + PART_NAMES[secs[i].part % 6];
				float tw = 70.f;
				roundRect(vg, x + w / 2 - tw / 2, y - 13, tw, 10, 1.5f, BTN);
				text(vg, f, x + w / 2, y - 8, lab, 6.5f, WHITE, NVG_ALIGN_CENTER, 0.5f);
			}
			x += w;
		}
		if (gm && gm->running.load()) {
			float pos = (gm->pubSongBar.load() + gm->pubStepInBar.load() / 16.f) / total;
			float px = x0 + (x1 - x0) * std::min(1.f, pos);
			roundRect(vg, px - 0.8f, y - 3, 1.6f, h + 6, 0.5f, WHITE);
		}
		// bottom line
		int part = gm ? gm->pubPart.load() : 0;
		std::string bl;
		if (gm && inMain) {
			bl = string::f("MAIN: THE DROP IN A LOOP (THE THEME)    BAR %d / 8", gm->running.load() ? gm->pubSecBar.load() + 1 : 1);
		}
		else if (gm) {
			int songB = total;
			float mins = songB * 4.f * 60.f / std::max(20.f, bpm);
			bl = string::f("BAR %d / %d    %s %d / %d", gm->pubSongBar.load() + 1, songB, PART_NAMES[part],
			               gm->pubSecBar.load() + 1, gm->pubSecBars.load());
			if (part == P_BUILD)
				bl += std::string(": ") + BUILD_NAMES[gm->pubBuild.load()];
			bl += string::f("    SONG %d:%02d", (int) mins / 60, (int) mins % 60);
		}
		text(vg, f, 10, H - 8, bl, 8.f, DIM, NVG_ALIGN_LEFT, 0.6f);
		if (gm && inMain)
			text(vg, f, W - 52, H - 8, "LIVE SONG OR A CLICK ON A PART: THE SONG GOES ON", 7.f, YEL, NVG_ALIGN_RIGHT, 0.5f);
		else if (gm) {
			int np = gm->pubNextPart.load();
			text(vg, f, W - 52, H - 8, std::string("NEXT: ") + PART_NAMES[np % 6], 8.f, PART_COL[np % 6], NVG_ALIGN_RIGHT, 0.8f);
		}
	}

	void step() override {
		ModuleWidget::step();
		if (!gm)
			return;
		float dt = APP->window->getLastFrameDuration();
		if (flashT > 0.f)
			flashT -= dt;
		delete gm->spent.exchange(nullptr);
		int nr = gm->newReq.exchange(0);
		if (nr) {
			if (nr & (1 << 5))
				regenerate(-1);
			else
				for (int t = 0; t < NUM_TRACKS; t++)
					if (nr & (1 << t))
						regenerate(t);
		}
		int mr = gm->mutateReq.exchange(0);
		if (mr)
			mutateTracks(mr, 0.3f);
		// the LEN knobs act at once on the pattern
		for (int t = 0; t < NUM_TRACKS; t++) {
			float v = gm->params[GenraTechno::LEN_PARAM + t].getValue();
			if (v != lastLen[t]) {
				if (lastLen[t] >= 0.f && v > 0.f)
					gm->pat.len[t] = (uint8_t) clamp((int) v, 1, 64);
				lastLen[t] = v;
			}
		}
		// song length and tempo: a new plan
		float len = gm->params[GenraTechno::LENGTH_PARAM].getValue(), bp = gm->params[GenraTechno::BPM_PARAM].getValue();
		if (std::fabs(len - lastLength) > 0.25f || std::fabs(bp - lastBpm) > 4.f) {
			if (lastLength >= 0.f)
				gm->rebuildPlan();
			lastLength = len;
			lastBpm = bp;
		}
		// a knob turned: its value in the display for a moment
		for (int i = 0; i < GenraTechno::NUM_PARAMS; i++) {
			float v = gm->params[i].getValue();
			if (paramsSeen && v != lastParam[i]) {
				engine::ParamQuantity* pq = gm->paramQuantities[i];
				flash = pq ? string::uppercase(pq->getLabel()) + ": " + pq->getDisplayValueString() + pq->getUnit() : "";
				if (i >= GenraTechno::LEN_PARAM && i < GenraTechno::LEN_PARAM + 5 && v == 0.f)
					flash = std::string(TRACK_NAMES[i - GenraTechno::LEN_PARAM]) + " LOOP LENGTH: THE PATTERN'S";
				flashT = 1.5f;
			}
			lastParam[i] = v;
		}
		paramsSeen = true;
	}

	void appendContextMenu(Menu* menu) override {
		if (!gm)
			return;
		menu->addChild(new MenuSeparator);
		menu->addChild(createMenuItem("Export the pattern as MIDI", "", [=]() {
			std::vector<const Pattern*> ps = {&gm->pat};
			std::vector<uint8_t> d = songMidi(ps, std::vector<Section>(), gm->bpm(), gm->params[GenraTechno::ENERGY_PARAM].getValue(), false);
			std::string err;
			midifolder::save(std::string("GENRA TECHNO ") + styles()[gm->style].name, d, &err);
		}));
		menu->addChild(createMenuItem("Export the LIVE song as MIDI", "", [=]() {
			std::vector<Section> s;
			{
				std::lock_guard<std::mutex> lk(gm->viewMx);
				s = gm->view.sections;
			}
			std::vector<const Pattern*> ps = {&gm->pat};
			for (Section& x : s)
				x.pad = 0;
			std::vector<uint8_t> d = songMidi(ps, s, gm->bpm(), gm->params[GenraTechno::ENERGY_PARAM].getValue(), true);
			std::string err;
			midifolder::save(std::string("GENRA TECHNO song ") + styles()[gm->style].name, d, &err);
		}));
		menu->addChild(createBoolMenuItem("LIVE song: DJ intro and outro (32 bars, drums only)", "",
		                                  [=]() { return gm->liveDj; },
		                                  [=](bool v) {
			                                  gm->liveDj = v;
			                                  gm->rebuildPlan();
		                                  }));
		midifolder::appendMenu(menu);
	}
};

Model* modelGenraTechno = createModel<GenraTechno, GenraTechnoWidget>("GenraTechno");
