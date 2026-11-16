// SPDX-License-Identifier: GPL-3.0-only
// GENRA TECHNO ABLAUF: patterns on 64 pads (4 banks of 16) and a song list of rows (pad, bars, part, build). In SONG
// mode it hands GENRA TECHNO a script (rows plus the pads' patterns, SongScript); GENRA TECHNO plays it in time and
// tells back where it is. In PATT mode GENRA TECHNO plays its own LIVE song and a pad tap loads a pattern into it.
// The variation keys make a variant of the last pattern for a part (press again: the next one), MAKE SONG builds a
// song in a club form from the pads (variants made where a part has none).
#include "TechnoModule.hpp"
#include "TechnoUI.hpp"
#include "TechnoAblaufLayout.hpp"
#include "MidiFolder.hpp"
#include <algorithm>
#include <cmath>

using namespace tg;
namespace L = technoablauf_layout;

static const int BARS_CHOICES[7] = {4, 8, 16, 24, 32, 48, 64};
struct Form {
	const char* name;
	float minutes;
	bool dj;
};
static const Form FORMS[6] = {{"PEAK TIME · 6 MIN", 6.f, true},      {"DJ TOOL · 7 MIN", 7.f, true},
                              {"HYPNOTIC · 8 MIN", 8.f, true},       {"MELODIC JOURNEY · 7 MIN", 7.f, true},
                              {"RADIO EDIT · 3:30", 3.5f, false},    {"SHORT · 4 MIN", 4.f, false}};

struct Pad {
	bool used = false;
	Pattern pat;
	std::string name;
	int part = -1; // -1: a pattern from GENRA TECHNO as it was (TECHNO n)
};

struct TechnoAblauf : Module {
	enum ParamId { ROW_PARAM, PAD_PARAM, BARS_PARAM, PART_PARAM, NUM_PARAMS };
	enum InputId { RUN_INPUT, RST_INPUT, NEXT_INPUT, PADCV_INPUT, ROWCV_INPUT, NUM_INPUTS };
	enum OutputId { END_OUTPUT, ROW_OUTPUT, PART_OUTPUT, BUILD_OUTPUT, DROP_OUTPUT, PHRASE_OUTPUT, NUM_OUTPUTS };
	TechnoLink link;
	Pad pads[64];
	std::vector<Section> rows;
	int mode = 0; // 0 PATT, 1 SONG
	int bank = 0, selRow = 0, lastPad = -1, form = 0;
	bool dj = true, loop = false;
	std::atomic<bool> dirty{true};
	std::atomic<int> padCvReq{-1}, rowCvReq{-1};
	// audio
	dsp::SchmittTrigger runTrig, rstTrig, nextTrig;
	dsp::PulseGenerator endP, rowP, dropP, phraseP;
	int lastRow = -1, lastPart = -1, lastPadCv = -1, lastRowCv = -1;
	int64_t lastBarCount = -1;

	TechnoAblauf() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, 0);
		configParam(ROW_PARAM, 0.f, 127.f, 0.f, "Row", "", 0.f, 1.f, 1.f);
		paramQuantities[ROW_PARAM]->snapEnabled = true;
		configParam(PAD_PARAM, 0.f, 63.f, 0.f, "Pad of the row", "", 0.f, 1.f, 1.f);
		paramQuantities[PAD_PARAM]->snapEnabled = true;
		configSwitch(BARS_PARAM, 0.f, 6.f, 2.f, "Bars of the row", {"4", "8", "16", "24", "32", "48", "64"});
		configSwitch(PART_PARAM, 0.f, 5.f, 1.f, "Part of the row", {"Intro", "Groove", "Build", "Drop", "Break", "Outro"});
		configInput(RUN_INPUT, "Run (trigger: play / stop the song)");
		configInput(RST_INPUT, "Reset (to the first row)");
		configInput(NEXT_INPUT, "Next row (trigger, at the next bar)");
		configInput(PADCV_INPUT, "Pad CV (0..10 V: pads 1..64, PATT mode)");
		configInput(ROWCV_INPUT, "Row CV (0..10 V: selects a row)");
		configOutput(END_OUTPUT, "End of the song (trigger)");
		configOutput(ROW_OUTPUT, "New row (trigger)");
		configOutput(PART_OUTPUT, "Part (0 V intro, 1 groove, 2 build, 3 drop, 4 break, 5 outro)");
		configOutput(BUILD_OUTPUT, "Build (gate)");
		configOutput(DROP_OUTPUT, "Drop (trigger)");
		configOutput(PHRASE_OUTPUT, "Phrase (a trigger every 8 bars)");
		link.owner = this;
	}
	~TechnoAblauf() { unpublish(); }
	void onRemove(const RemoveEvent& e) override { unpublish(); }
	void unpublish() {
		if (GenraTechno* g = link.main.load()) {
			delete g->script.exchange(nullptr);
			g->updateView();
		}
	}

	void process(const ProcessArgs& args) override {
		GenraTechno* g = link.main.load();
		if (runTrig.process(inputs[RUN_INPUT].getVoltage(), 0.1f, 1.f) && g) {
			if (g->running.load())
				g->stopReq = true;
			else
				g->startRow = 0;
		}
		if (rstTrig.process(inputs[RST_INPUT].getVoltage(), 0.1f, 1.f) && g && g->running.load())
			g->startRow = 0;
		if (nextTrig.process(inputs[NEXT_INPUT].getVoltage(), 0.1f, 1.f) && g)
			g->nextRowReq = true;
		if (inputs[PADCV_INPUT].isConnected()) {
			int p = clamp((int) std::round(inputs[PADCV_INPUT].getVoltage() * 6.3f), 0, 63);
			if (p != lastPadCv)
				padCvReq = p;
			lastPadCv = p;
		}
		if (inputs[ROWCV_INPUT].isConnected()) {
			int r = clamp((int) std::round(inputs[ROWCV_INPUT].getVoltage() * 6.3f), 0, 63);
			if (r != lastRowCv)
				rowCvReq = r;
			lastRowCv = r;
		}
		if (g && g->running.load()) {
			int row = g->pubRow.load(), part = g->pubPart.load();
			if (row != lastRow) {
				if (row >= 0)
					rowP.trigger(1e-3f);
				if (row == 0 && lastRow > 0)
					endP.trigger(1e-3f);
				lastRow = row;
			}
			if (part != lastPart && part == P_DROP)
				dropP.trigger(1e-3f);
			lastPart = part;
			int64_t bc = g->pubBarCount.load();
			if (bc != lastBarCount) {
				if (g->pubSecBar.load() % 8 == 0)
					phraseP.trigger(1e-3f);
				lastBarCount = bc;
			}
		}
		float st = args.sampleTime;
		outputs[END_OUTPUT].setVoltage(endP.process(st) ? 10.f : 0.f);
		outputs[ROW_OUTPUT].setVoltage(rowP.process(st) ? 10.f : 0.f);
		outputs[DROP_OUTPUT].setVoltage(dropP.process(st) ? 10.f : 0.f);
		outputs[PHRASE_OUTPUT].setVoltage(phraseP.process(st) ? 10.f : 0.f);
		outputs[PART_OUTPUT].setVoltage(g ? (float) g->pubPart.load() : 0.f);
		outputs[BUILD_OUTPUT].setVoltage(g && g->running.load() && g->pubPart.load() == P_BUILD ? 10.f : 0.f);
	}

	json_t* dataToJson() override {
		json_t* root = json_object();
		link.toJson(root);
		json_object_set_new(root, "mode", json_integer(mode));
		json_object_set_new(root, "bank", json_integer(bank));
		json_object_set_new(root, "selRow", json_integer(selRow));
		json_object_set_new(root, "lastPad", json_integer(lastPad));
		json_object_set_new(root, "form", json_integer(form));
		json_object_set_new(root, "dj", json_boolean(dj));
		json_object_set_new(root, "loop", json_boolean(loop));
		json_t* pa = json_array();
		for (int i = 0; i < 64; i++) {
			json_t* o = json_object();
			if (pads[i].used) {
				json_object_set_new(o, "name", json_string(pads[i].name.c_str()));
				json_object_set_new(o, "part", json_integer(pads[i].part));
				json_object_set_new(o, "pattern", json_string(string::toBase64((const uint8_t*) &pads[i].pat, sizeof(Pattern)).c_str()));
			}
			json_array_append_new(pa, o);
		}
		json_object_set_new(root, "pads", pa);
		json_t* ro = json_array();
		for (const Section& s : rows) {
			json_t* a = json_array();
			int v[6] = {s.part, s.bars, s.build, s.grow, s.dropIdx, s.pad};
			for (int k = 0; k < 6; k++)
				json_array_append_new(a, json_integer(v[k]));
			json_array_append_new(ro, a);
		}
		json_object_set_new(root, "rows", ro);
		return root;
	}
	void dataFromJson(json_t* root) override {
		link.fromJson(root);
		auto geti = [&](const char* k, int def) {
			json_t* j = json_object_get(root, k);
			return j ? (int) json_integer_value(j) : def;
		};
		mode = clamp(geti("mode", 0), 0, 1);
		bank = clamp(geti("bank", 0), 0, 3);
		selRow = std::max(0, geti("selRow", 0));
		lastPad = clamp(geti("lastPad", -1), -1, 63);
		form = clamp(geti("form", 0), 0, 5);
		dj = !json_is_false(json_object_get(root, "dj"));
		loop = json_is_true(json_object_get(root, "loop"));
		if (json_t* pa = json_object_get(root, "pads"))
			for (int i = 0; i < 64 && i < (int) json_array_size(pa); i++) {
				json_t* o = json_array_get(pa, i);
				json_t* pj = json_object_get(o, "pattern");
				pads[i] = Pad();
				if (!pj)
					continue;
				std::vector<uint8_t> b = string::fromBase64(json_string_value(pj));
				if (b.size() != sizeof(Pattern))
					continue;
				std::memcpy((void*) &pads[i].pat, b.data(), sizeof(Pattern));
				pads[i].used = true;
				if (json_t* n = json_object_get(o, "name"))
					pads[i].name = json_string_value(n);
				if (json_t* n = json_object_get(o, "part"))
					pads[i].part = (int) json_integer_value(n);
			}
		rows.clear();
		if (json_t* ro = json_object_get(root, "rows"))
			for (size_t i = 0; i < json_array_size(ro); i++) {
				json_t* a = json_array_get(ro, i);
				Section s;
				int v[6] = {};
				for (int k = 0; k < 6 && k < (int) json_array_size(a); k++)
					v[k] = (int) json_integer_value(json_array_get(a, k));
				s.part = (uint8_t) clamp(v[0], 0, 5);
				s.bars = (uint8_t) clamp(v[1], 1, 255);
				s.build = (uint8_t) clamp(v[2], 0, NUM_BUILDS - 1);
				s.grow = (uint8_t) clamp(v[3], 0, 2);
				s.dropIdx = (uint8_t) clamp(v[4], 0, 255);
				s.pad = (uint8_t) clamp(v[5], 0, 63);
				rows.push_back(s);
			}
		dirty = true;
	}
};

// ================================================================ the panel
struct TechnoAblaufWidget : ModuleWidget {
	TechnoAblauf* mod = nullptr;
	struct Snap {
		std::vector<Section> rows;
		std::vector<Pad> pads;
	};
	std::deque<Snap> undo;
	std::vector<std::pair<SongScript*, float>> trash;
	int armed = 0; // 1 copy (source), 2 copy (target), 3 clear, 4 clear all (press again)
	int copySrc = -1;
	float armT = 0.f;
	float last[TechnoAblauf::NUM_PARAMS] = {-1, -1, -1, -1};
	int scroll = 0;
	GenraTechno* lastMain = nullptr;
	int lastPublishedMode = -1;

	GenraTechno* gm() { return mod ? mod->link.main.load() : nullptr; }

	void pushUndo() {
		if (!mod)
			return;
		Snap s;
		s.rows = mod->rows;
		s.pads.assign(mod->pads, mod->pads + 64);
		undo.push_back(s);
		while (undo.size() > 12)
			undo.pop_front();
	}
	int freePad() {
		for (int k = 0; k < 64; k++) {
			int i = (mod->bank * 16 + k) % 64;
			if (!mod->pads[i].used)
				return i;
		}
		return -1;
	}
	int countPart(int part) {
		int n = 0;
		for (int i = 0; i < 64; i++)
			if (mod->pads[i].used && mod->pads[i].part == part)
				n++;
		return n;
	}
	/** Store GENRA TECHNO's pattern as it plays on a pad (TECHNO n). */
	int storeCurrent(int at) {
		GenraTechno* g = gm();
		if (!g || at < 0)
			return -1;
		Pad& p = mod->pads[at];
		p.used = true;
		p.pat = g->pat;
		p.part = -1;
		p.name = string::f("TECHNO %d", countPart(-1));
		mod->lastPad = at;
		mod->dirty = true;
		return at;
	}
	/** A variant of the last pattern for a part, on the next free pad. */
	int makeVariant(int part, uint32_t seed) {
		GenraTechno* g = gm();
		int at = freePad();
		if (!g || at < 0)
			return -1;
		Pattern base = (mod->lastPad >= 0 && mod->pads[mod->lastPad].used) ? mod->pads[mod->lastPad].pat : g->pat;
		// a variant is built from the original (TECHNO n) when the last pad is a variant itself
		if (mod->lastPad >= 0 && mod->pads[mod->lastPad].used && mod->pads[mod->lastPad].part >= 0)
			for (int i = 0; i < 64; i++)
				if (mod->pads[i].used && mod->pads[i].part < 0) {
					base = mod->pads[i].pat;
					break;
				}
		GenSettings gs = g->settings();
		gs.style = base.style;
		gs.key = base.key;
		Pattern p = base;
		auto newTrack = [&](int t, float densAdd) {
			GenSettings x = gs;
			x.dens[t] = clamp(x.dens[t] + densAdd, 0.f, 1.f);
			generate(p, x, hash(seed, t, part), t);
		};
		switch (part) {
		case P_INTRO:
		case P_OUTRO:
			newTrack(T_BASS, -0.2f);
			newTrack(T_STAB, -0.2f);
			break;
		case P_GROOVE:
			newTrack(T_BASS, 0.f);
			newTrack(T_STAB, 0.f);
			break;
		case P_BUILD:
			newTrack(T_LEAD, 0.f);
			newTrack(T_STAB, 0.1f);
			break;
		case P_DROP:
			newTrack(T_BASS, 0.15f);
			newTrack(T_STAB, 0.15f);
			newTrack(T_LEAD, 0.15f);
			break;
		case P_BREAK:
			newTrack(T_LEAD, 0.f);
			newTrack(T_ATMO, 0.f);
			break;
		}
		Pad& d = mod->pads[at];
		d.used = true;
		d.pat = p;
		d.part = part;
		d.name = string::f("%s %d", PART_NAMES[part], countPart(part) + 1);
		mod->lastPad = at;
		mod->dirty = true;
		return at;
	}
	void makeSong() {
		GenraTechno* g = gm();
		if (!g)
			return;
		pushUndo();
		bool any = false;
		for (int i = 0; i < 64; i++)
			any = any || mod->pads[i].used;
		if (!any)
			storeCurrent(freePad());
		const Form& f = FORMS[mod->form];
		mod->dj = f.dj;
		std::vector<Section> plan = planSong(g->style, f.minutes, g->bpm(), random::u32(), g->forcedBuild, true, f.dj);
		int base = -1;
		for (int i = 0; i < 64 && base < 0; i++)
			if (mod->pads[i].used && mod->pads[i].part < 0)
				base = i;
		if (base < 0)
			for (int i = 0; i < 64 && base < 0; i++)
				if (mod->pads[i].used)
					base = i;
		int seen[NUM_PARTS] = {};
		mod->rows.clear();
		for (Section s : plan) {
			int part = s.part;
			// the k-th section of a part uses the k-th pad of that part (made if missing; GROOVE and BUILD may use the original)
			int k = seen[part]++;
			int found = -1, n = 0;
			for (int i = 0; i < 64; i++)
				if (mod->pads[i].used && mod->pads[i].part == part && n++ == k)
					found = i;
			if (found < 0) {
				if ((part == P_GROOVE || part == P_BUILD) && base >= 0)
					found = base;
				else {
					mod->lastPad = base;
					found = makeVariant(part, random::u32());
					if (found < 0)
						found = std::max(0, base);
				}
			}
			s.pad = (uint8_t) found;
			mod->rows.push_back(s);
		}
		mod->mode = 1;
		mod->selRow = 0;
		mod->dirty = true;
	}
	void publish() {
		GenraTechno* g = gm();
		if (!g)
			return;
		if (mod->mode == 1 && !mod->rows.empty()) {
			SongScript* s = new SongScript;
			s->rows = mod->rows;
			s->pads.resize(64);
			for (int i = 0; i < 64; i++)
				s->pads[i] = mod->pads[i].used ? mod->pads[i].pat : g->pat;
			s->djEdges = mod->dj;
			s->loopRow = mod->loop ? mod->selRow : -1;
			if (SongScript* old = g->script.exchange(s))
				trash.push_back(std::make_pair(old, 2.f));
		}
		else if (SongScript* old = g->script.exchange(nullptr))
			trash.push_back(std::make_pair(old, 2.f));
		g->updateView();
	}

	void padTap(int i) {
		GenraTechno* g = gm();
		Pad& p = mod->pads[i];
		if (armed == 1) {
			if (p.used) {
				copySrc = i;
				armed = 2;
			}
			return;
		}
		if (armed == 2) {
			pushUndo();
			mod->pads[i] = mod->pads[copySrc];
			armed = 0;
			mod->dirty = true;
			return;
		}
		if (armed == 3) {
			pushUndo();
			p = Pad();
			armed = 0;
			mod->dirty = true;
			return;
		}
		if (!p.used) {
			pushUndo();
			storeCurrent(i);
			return;
		}
		mod->lastPad = i;
		if (mod->mode == 0 && g)
			g->post(p.pat, !g->running.load());
	}

	TechnoAblaufWidget(TechnoAblauf* module) {
		setModule(module);
		mod = module;
		setPanel(createPanel(asset::plugin(pluginInstance, "res/TechnoAblauf.svg")));
		using namespace tui;
		addParam(createParamCentered<GLargeKnob>(mm(L::K_ROW), module, TechnoAblauf::ROW_PARAM));
		addParam(createParamCentered<GSmallKnob>(mm(L::K_PAD), module, TechnoAblauf::PAD_PARAM));
		addParam(createParamCentered<GSmallKnob>(mm(L::K_BARS), module, TechnoAblauf::BARS_PARAM));
		addParam(createParamCentered<GSmallKnob>(mm(L::K_PART), module, TechnoAblauf::PART_PARAM));
		static const float* const IN[5] = {L::IN_RUN, L::IN_RST, L::IN_NEXT, L::IN_PADCV, L::IN_ROWCV};
		for (int i = 0; i < 5; i++)
			addInput(createInputCentered<PJ301MPort>(mm(IN[i]), module, i));
		static const float* const OUT[6] = {L::OUT_END, L::OUT_ROW, L::OUT_PART, L::OUT_BUILD, L::OUT_DROP, L::OUT_PHRASE};
		for (int i = 0; i < 6; i++)
			addOutput(createOutputCentered<PJ301MPort>(mm(OUT[i]), module, i));

		TButton* bp = place<TButton>(this, L::BTN_PATT);
		bp->label = "PATT";
		bp->fontSize = 7.5f;
		bp->colOf = [=]() { return mod && mod->mode == 0 ? dark(RED, 0.6f) : BTN; };
		bp->act = [=]() {
			if (mod) {
				mod->mode = 0;
				mod->dirty = true;
			}
		};
		TButton* bs = place<TButton>(this, L::BTN_SONG);
		bs->label = "SONG";
		bs->fontSize = 7.5f;
		bs->colOf = [=]() { return mod && mod->mode == 1 ? dark(RED, 0.6f) : BTN; };
		bs->act = [=]() {
			if (mod) {
				mod->mode = 1;
				mod->dirty = true;
			}
		};
		static const float* const BK[4] = {L::BTN_BANK0, L::BTN_BANK1, L::BTN_BANK2, L::BTN_BANK3};
		for (int b = 0; b < 4; b++) {
			TButton* k = place<TButton>(this, BK[b]);
			k->label = std::string(1, (char) ('A' + b));
			k->fontSize = 8.f;
			k->colOf = [=]() { return mod && mod->bank == b ? dark(RED, 0.6f) : BTN; };
			k->act = [=]() {
				if (mod)
					mod->bank = b;
			};
		}
		static const float* const VB[6] = {L::BTN_VAR0, L::BTN_VAR1, L::BTN_VAR2, L::BTN_VAR3, L::BTN_VAR4, L::BTN_VAR5};
		for (int i = 0; i < 6; i++) {
			TButton* v = place<TButton>(this, VB[i]);
			v->label = PART_NAMES[i];
			v->fontSize = 7.f;
			v->col = dark(PART_COL[i], 0.55f);
			v->textCol = WHITE;
			v->act = [=]() {
				if (!mod)
					return;
				pushUndo();
				makeVariant(i, random::u32());
			};
		}
		TField* fo = place<TField>(this, L::F_FORM);
		fo->prefix = "FORM";
		fo->value = [=]() { return std::string(FORMS[mod ? mod->form : 0].name); };
		fo->count = []() { return 6; };
		fo->current = [=]() { return mod ? mod->form : 0; };
		fo->item = [](int i) { return std::string(FORMS[i].name); };
		fo->choose = [=](int i) {
			if (mod)
				mod->form = i;
		};
		TButton* mk = place<TButton>(this, L::BTN_MAKE);
		mk->label = "MAKE SONG";
		mk->col = RED;
		mk->textCol = WHITE;
		mk->act = [=]() {
			if (mod)
				makeSong();
		};
		TButton* dj = place<TButton>(this, L::BTN_DJ);
		dj->label = "DJ IN/OUT";
		dj->fontSize = 7.f;
		dj->lit = [=]() { return mod && mod->dj; };
		dj->colOf = [=]() { return mod && mod->dj ? dark(RED, 0.5f) : BTN; };
		dj->act = [=]() {
			if (mod) {
				mod->dj = !mod->dj;
				mod->dirty = true;
			}
		};
		TButton* cp = place<TButton>(this, L::BTN_COPY);
		cp->labelOf = [=]() { return std::string(armed == 1 ? "FROM?" : (armed == 2 ? "TO?" : "COPY")); };
		cp->fontSize = 7.f;
		cp->lit = [=]() { return armed == 1 || armed == 2; };
		cp->act = [=]() {
			armed = armed == 1 || armed == 2 ? 0 : 1;
			armT = 6.f;
		};
		TButton* cl = place<TButton>(this, L::BTN_CLEAR);
		cl->labelOf = [=]() { return std::string(armed == 3 ? "WHICH?" : "CLEAR"); };
		cl->fontSize = 7.f;
		cl->lit = [=]() { return armed == 3; };
		cl->act = [=]() {
			armed = armed == 3 ? 0 : 3;
			armT = 6.f;
		};
		TButton* ca = place<TButton>(this, L::BTN_CLEARALL);
		ca->labelOf = [=]() { return std::string(armed == 4 ? "SURE?" : "CLEAR ALL"); };
		ca->fontSize = 7.f;
		ca->lit = [=]() { return armed == 4; };
		ca->act = [=]() {
			if (!mod)
				return;
			if (armed == 4) {
				pushUndo();
				for (int i = 0; i < 64; i++)
					mod->pads[i] = Pad();
				mod->lastPad = -1;
				armed = 0;
				mod->dirty = true;
			}
			else {
				armed = 4;
				armT = 4.f;
			}
		};
		TButton* pl = place<TButton>(this, L::BTN_PLAY);
		pl->label = "PLAY";
		pl->col = RED;
		pl->textCol = WHITE;
		pl->fontSize = 10.f;
		pl->lit = [=]() { return gm() && gm()->running.load() && mod->mode == 1; };
		pl->act = [=]() {
			GenraTechno* g = gm();
			if (!g || !mod)
				return;
			if (mod->rows.empty())
				makeSong();
			mod->mode = 1;
			publish();
			mod->dirty = false;
			g->startRow = clamp(mod->selRow, 0, (int) mod->rows.size() - 1);
		};
		TButton* ins = place<TButton>(this, L::BTN_INSERT);
		ins->label = "INSERT";
		ins->fontSize = 7.f;
		ins->act = [=]() {
			if (!mod)
				return;
			pushUndo();
			Section s;
			s.bars = 16;
			int pad = mod->lastPad >= 0 ? mod->lastPad : 0;
			s.pad = (uint8_t) pad;
			s.part = (uint8_t) (mod->pads[pad].part >= 0 ? mod->pads[pad].part : P_GROOVE);
			if (s.part == P_BUILD)
				s.build = (uint8_t) pickBuild(mod->pads[pad].pat.style, random::u32(), -1, 0);
			int at = mod->rows.empty() ? 0 : std::min((int) mod->rows.size(), mod->selRow + 1);
			mod->rows.insert(mod->rows.begin() + at, s);
			mod->selRow = at;
			mod->params[TechnoAblauf::ROW_PARAM].setValue(at);
			mod->dirty = true;
		};
		TButton* del = place<TButton>(this, L::BTN_DELETE);
		del->label = "DELETE";
		del->fontSize = 7.f;
		del->act = [=]() {
			if (!mod || mod->rows.empty())
				return;
			pushUndo();
			int r = clamp(mod->selRow, 0, (int) mod->rows.size() - 1);
			mod->rows.erase(mod->rows.begin() + r);
			mod->selRow = std::max(0, std::min(r, (int) mod->rows.size() - 1));
			mod->dirty = true;
		};
		TButton* un = place<TButton>(this, L::BTN_UNDO);
		un->label = "UNDO";
		un->fontSize = 7.f;
		un->act = [=]() {
			if (!mod || undo.empty())
				return;
			mod->rows = undo.back().rows;
			for (int i = 0; i < 64 && i < (int) undo.back().pads.size(); i++)
				mod->pads[i] = undo.back().pads[i];
			undo.pop_back();
			mod->dirty = true;
		};
		TButton* sp = place<TButton>(this, L::BTN_STOP);
		sp->label = "STOP";
		sp->fontSize = 7.f;
		sp->act = [=]() {
			if (GenraTechno* g = gm())
				g->stopReq = true;
		};
		TButton* lp = place<TButton>(this, L::BTN_LOOP);
		lp->label = "LOOP";
		lp->fontSize = 7.f;
		lp->lit = [=]() { return mod && mod->loop; };
		lp->act = [=]() {
			if (mod) {
				mod->loop = !mod->loop;
				mod->dirty = true;
			}
		};
		TButton* es = place<TButton>(this, L::BTN_EXPSONG);
		es->label = "EXPORT SONG";
		es->fontSize = 7.f;
		es->act = [=]() {
			GenraTechno* g = gm();
			if (!g || !mod || mod->rows.empty())
				return;
			std::vector<const Pattern*> ps(64);
			for (int i = 0; i < 64; i++)
				ps[i] = mod->pads[i].used ? &mod->pads[i].pat : &g->pat;
			std::vector<uint8_t> d = songMidi(ps, mod->rows, g->bpm(), g->params[GenraTechno::ENERGY_PARAM].getValue(), mod->dj);
			std::string err;
			midifolder::save("GENRA TECHNO ABLAUF song", d, &err);
		};
		TButton* ep = place<TButton>(this, L::BTN_EXPPAD);
		ep->label = "EXPORT PAD";
		ep->fontSize = 7.f;
		ep->act = [=]() {
			GenraTechno* g = gm();
			if (!g || !mod)
				return;
			const Pattern* p = mod->lastPad >= 0 && mod->pads[mod->lastPad].used ? &mod->pads[mod->lastPad].pat : &g->pat;
			std::vector<const Pattern*> ps = {p};
			std::vector<uint8_t> d = songMidi(ps, std::vector<Section>(), g->bpm(), g->params[GenraTechno::ENERGY_PARAM].getValue(), false);
			std::string err;
			midifolder::save(mod->lastPad >= 0 ? "GENRA TECHNO " + mod->pads[mod->lastPad].name : "GENRA TECHNO pattern", d, &err);
		};
		// the song list
		Drawn* sl = place<Drawn>(this, L::DISP_SONG);
		sl->paintLit = [=](const DrawArgs& a) { drawSong(a.vg, sl->box.size); };
		struct Click : widget::OpaqueWidget {
			std::function<void(Vec, int)> on;
			std::function<void(float)> wheel;
			void onButton(const ButtonEvent& e) override {
				if (e.action == GLFW_PRESS && on) {
					on(e.pos, e.button);
					e.consume(this);
				}
			}
			void onHoverScroll(const HoverScrollEvent& e) override {
				if (wheel) {
					wheel(e.scrollDelta.y);
					e.consume(this);
				}
			}
		};
		Click* slc = new Click;
		slc->box = sl->box;
		slc->on = [=](Vec p, int) {
			if (!mod)
				return;
			int r = scroll + (int) ((p.y - 18.f) / 13.f);
			if (r >= 0 && r < (int) mod->rows.size()) {
				mod->selRow = r;
				mod->params[TechnoAblauf::ROW_PARAM].setValue(r);
			}
		};
		slc->wheel = [=](float d) { scroll = std::max(0, scroll + (d > 0 ? -1 : 1)); };
		addChild(slc);
		// the pads
		Drawn* pd = place<Drawn>(this, L::A_PADS);
		pd->paintLit = [=](const DrawArgs& a) { drawPads(a.vg, pd->box.size); };
		Click* pdc = new Click;
		pdc->box = pd->box;
		pdc->on = [=](Vec p, int button) {
			if (!mod)
				return;
			float gap = mm2px(1.4f);
			float w = (pd->box.size.x - 3 * gap) / 4, h = (pd->box.size.y - 3 * gap) / 4;
			int cx = (int) (p.x / (w + gap)), cy = (int) (p.y / (h + gap));
			if (cx < 0 || cx > 3 || cy < 0 || cy > 3)
				return;
			int i = mod->bank * 16 + cy * 4 + cx;
			if (button == GLFW_MOUSE_BUTTON_RIGHT) {
				Pad& pp = mod->pads[i];
				ui::Menu* menu = createMenu();
				menu->addChild(createMenuLabel(pp.used ? pp.name : "Empty pad"));
				if (pp.used) {
					menu->addChild(createMenuItem("Export as MIDI", "", [=]() {
						GenraTechno* g = gm();
						std::vector<const Pattern*> ps = {&mod->pads[i].pat};
						std::vector<uint8_t> d = songMidi(ps, std::vector<Section>(), g ? g->bpm() : 130.f, 0.7f, false);
						std::string err;
						midifolder::save("GENRA TECHNO " + mod->pads[i].name, d, &err);
					}));
					for (int t = 0; t < NUM_TRACKS; t++)
						menu->addChild(createMenuItem(std::string("Export only ") + TRACK_NAMES[t], "", [=]() {
							GenraTechno* g = gm();
							std::vector<const Pattern*> ps = {&mod->pads[i].pat};
							std::vector<uint8_t> d = songMidi(ps, std::vector<Section>(), g ? g->bpm() : 130.f, 0.7f, false, t);
							std::string err;
							midifolder::save("GENRA TECHNO " + mod->pads[i].name + " " + TRACK_NAMES[t], d, &err);
						}));
					menu->addChild(createMenuItem("Store GENRA TECHNO's pattern here", "", [=]() {
						pushUndo();
						storeCurrent(i);
					}));
				}
				return;
			}
			padTap(i);
		};
		addChild(pdc);
		Drawn* ln = place<Drawn>(this, L::A_LINK);
		ln->paintLit = [=](const DrawArgs& a) {
			bool on = gm() != nullptr;
			nvgBeginPath(a.vg);
			nvgCircle(a.vg, 6, ln->box.size.y / 2, 3.5f);
			nvgFillColor(a.vg, on ? GREEN : nvgRGB(0x44, 0x46, 0x50));
			nvgFill(a.vg);
			text(a.vg, bold(), 14, ln->box.size.y / 2, on ? "LINK" : "NO LINK", 8.f, on ? TXT : DIM);
		};
	}

	void drawSong(NVGcontext* vg, Vec sz) {
		using namespace tui;
		auto f = bold();
		GenraTechno* g = gm();
		text(vg, f, 6, 9, "#", 7.f, DIM);
		text(vg, f, 24, 9, "PAD", 7.f, DIM);
		text(vg, f, sz.x * 0.62f, 9, "BARS", 7.f, DIM);
		text(vg, f, sz.x * 0.76f, 9, "PART", 7.f, DIM);
		if (!mod)
			return;
		int n = (int) mod->rows.size();
		int vis = std::max(1, (int) ((sz.y - 34.f) / 13.f));
		if (mod->selRow < scroll)
			scroll = mod->selRow;
		if (mod->selRow >= scroll + vis)
			scroll = mod->selRow - vis + 1;
		scroll = clamp(scroll, 0, std::max(0, n - vis));
		int playing = g && g->running.load() && g->pubSongMode.load() ? g->pubRow.load() : -1;
		if (n == 0)
			text(vg, f, sz.x / 2, sz.y / 2, "NO SONG YET: MAKE SONG, OR A PAD + INSERT", 7.5f, DIM, NVG_ALIGN_CENTER);
		for (int i = 0; i < vis && scroll + i < n; i++) {
			int r = scroll + i;
			const Section& s = mod->rows[r];
			float y = 18.f + i * 13.f;
			if (r == playing)
				roundRect(vg, 2, y - 1, sz.x - 4, 12, 1.5f, dark(RED, 0.45f));
			else if (r == mod->selRow)
				roundRect(vg, 2, y - 1, sz.x - 4, 12, 1.5f, nvgRGB(0x26, 0x28, 0x30));
			if (r == mod->selRow)
				roundRect(vg, 2, y - 1, 2, 12, 0.5f, WHITE);
			NVGcolor pc = PART_COL[s.part % 6];
			text(vg, f, 6, y + 5, string::f("%02d", r + 1), 8.f, r == playing ? RED : DIM);
			roundRect(vg, 20, y + 1, 2.5f, 8, 0.6f, pc);
			const Pad& p = mod->pads[s.pad % 64];
			std::string nm = string::f("%c%d ", 'A' + s.pad / 16, s.pad % 16 + 1) + (p.used ? p.name : "(empty)");
			textFit(vg, f, 26, y + 5, nm, 8.f, sz.x * 0.55f, TXT);
			text(vg, f, sz.x * 0.65f, y + 5, string::f("%d", s.bars), 8.f, TXT, NVG_ALIGN_CENTER);
			std::string pt = PART_NAMES[s.part % 6];
			if (s.part == P_BUILD)
				pt += std::string(" ") + BUILD_NAMES[s.build % NUM_BUILDS];
			if (mod->loop && r == mod->selRow)
				pt += " LOOP";
			textFit(vg, f, sz.x * 0.76f, y + 5, pt, 7.f, sz.x * 0.23f, pc);
		}
		// bottom line
		int total = songBars(mod->rows);
		float bpm = g ? g->bpm() : 130.f;
		float mins = total * 4.f * 60.f / std::max(20.f, bpm);
		std::string bl = string::f("%d ROWS · %d BARS · %d:%02d @ %.0f", n, total, (int) mins / 60, (int) mins % 60, bpm);
		if (g && g->running.load() && g->pubSongMode.load())
			bl = string::f("BAR %d / %d · ROW %d / %d", g->pubSongBar.load() + 1, total, g->pubRow.load() + 1, n);
		text(vg, f, sz.x - 6, sz.y - 7, bl, 7.f, DIM, NVG_ALIGN_RIGHT);
		text(vg, f, 6, sz.y - 7, mod->mode == 1 ? "SONG" : "PATT", 7.f, mod->mode == 1 ? RED : DIM);
	}

	void drawPads(NVGcontext* vg, Vec sz) {
		using namespace tui;
		if (!mod)
			return;
		auto f = bold();
		GenraTechno* g = gm();
		float gap = mm2px(1.4f);
		float w = (sz.x - 3 * gap) / 4, h = (sz.y - 3 * gap) / 4;
		int playingPad = -1;
		if (g && g->running.load() && g->pubSongMode.load()) {
			int r = g->pubRow.load();
			if (r >= 0 && r < (int) mod->rows.size())
				playingPad = mod->rows[r].pad;
		}
		for (int k = 0; k < 16; k++) {
			int i = mod->bank * 16 + k;
			const Pad& p = mod->pads[i];
			float x = (k % 4) * (w + gap), y = (k / 4) * (h + gap);
			NVGcolor c = p.used ? (p.part >= 0 ? PART_COL[p.part % 6] : TXT) : nvgRGB(0x44, 0x46, 0x4f);
			roundRect(vg, x, y, w, h, 2.f, p.used ? dark(c, 0.22f) : nvgRGB(0x1a, 0x1b, 0x20));
			if (p.used)
				roundRect(vg, x + 3, y + 2.5f, w - 6, 2.2f, 0.8f, c);
			text(vg, f, x + 4, y + 10, string::f("%c%d", 'A' + mod->bank, k + 1), 6.5f, DIM);
			if (p.used)
				textFit(vg, f, x + 4, y + h - 9, p.name, 7.5f, w - 8, WHITE);
			if (i == playingPad || (i == mod->lastPad && mod->mode == 0))
				frame(vg, 0.8f + x, 0.8f + y, w - 1.6f, h - 1.6f, 2.f, WHITE, 1.4f);
			if ((armed == 2 && i == copySrc))
				frame(vg, 0.8f + x, 0.8f + y, w - 1.6f, h - 1.6f, 2.f, YEL, 1.4f);
		}
	}

	void step() override {
		ModuleWidget::step();
		if (!mod)
			return;
		float dt = APP->window->getLastFrameDuration();
		mod->link.uiStep(dt);
		GenraTechno* g = gm();
		// the old scripts: deleted a little later (the player may still be in one)
		for (auto& t : trash)
			t.second -= dt;
		for (size_t i = 0; i < trash.size();)
			if (trash[i].second <= 0.f) {
				delete trash[i].first;
				trash.erase(trash.begin() + i);
			}
			else
				i++;
		if (armed && (armT -= dt) <= 0.f)
			armed = 0;
		if (g != lastMain) {
			lastMain = g;
			mod->dirty = true;
		}
		// CV requests
		int pr = mod->padCvReq.exchange(-1);
		if (pr >= 0 && mod->pads[pr].used) {
			mod->lastPad = pr;
			if (mod->mode == 0 && g)
				g->post(mod->pads[pr].pat, !g->running.load());
		}
		int rr = mod->rowCvReq.exchange(-1);
		if (rr >= 0 && !mod->rows.empty())
			mod->params[TechnoAblauf::ROW_PARAM].setValue(std::min(rr, (int) mod->rows.size() - 1));
		// the knobs: ROW picks a row and shows its pad, bars and part; those three change the row
		int nRows = (int) mod->rows.size();
		float rowV = mod->params[TechnoAblauf::ROW_PARAM].getValue();
		if (rowV != last[0]) {
			mod->selRow = clamp((int) rowV, 0, std::max(0, nRows - 1));
			if (nRows) {
				const Section& s = mod->rows[mod->selRow];
				mod->params[TechnoAblauf::PAD_PARAM].setValue(s.pad);
				int bi = 2;
				for (int k = 0; k < 7; k++)
					if (BARS_CHOICES[k] == s.bars)
						bi = k;
				mod->params[TechnoAblauf::BARS_PARAM].setValue(bi);
				mod->params[TechnoAblauf::PART_PARAM].setValue(s.part);
			}
			for (int i = 0; i < 4; i++)
				last[i] = mod->params[i].getValue();
			if (mod->loop)
				mod->dirty = true;
		}
		else if (nRows) {
			Section& s = mod->rows[clamp(mod->selRow, 0, nRows - 1)];
			bool ch = false;
			float v = mod->params[TechnoAblauf::PAD_PARAM].getValue();
			if (v != last[1]) {
				s.pad = (uint8_t) v;
				ch = true;
			}
			v = mod->params[TechnoAblauf::BARS_PARAM].getValue();
			if (v != last[2]) {
				s.bars = (uint8_t) BARS_CHOICES[clamp((int) v, 0, 6)];
				ch = true;
			}
			v = mod->params[TechnoAblauf::PART_PARAM].getValue();
			if (v != last[3]) {
				s.part = (uint8_t) v;
				if (s.part == P_BUILD && s.build == BT_NONE)
					s.build = (uint8_t) pickBuild(g ? g->style : 2, random::u32(), -1, 0);
				ch = true;
			}
			if (ch)
				mod->dirty = true;
			for (int i = 0; i < 4; i++)
				last[i] = mod->params[i].getValue();
		}
		if (mod->dirty.exchange(false))
			publish();
	}

	void appendContextMenu(Menu* menu) override {
		if (!mod)
			return;
		mod->link.appendMenu(menu);
		menu->addChild(new MenuSeparator);
		menu->addChild(createSubmenuItem("Build of the selected row", "", [=](ui::Menu* sub) {
			for (int b = 1; b < NUM_BUILDS; b++)
				sub->addChild(createCheckMenuItem(BUILD_NAMES[b], "",
				                                  [=]() {
					                                  int r = mod->selRow;
					                                  return r < (int) mod->rows.size() && mod->rows[r].build == b;
				                                  },
				                                  [=]() {
					                                  int r = mod->selRow;
					                                  if (r < (int) mod->rows.size()) {
						                                  mod->rows[r].build = (uint8_t) b;
						                                  mod->dirty = true;
					                                  }
				                                  }));
		}));
		menu->addChild(createSubmenuItem("GROW of the selected row", "", [=](ui::Menu* sub) {
			static const char* const G[3] = {"Normal", "Bigger (2nd)", "Biggest (3rd)"};
			for (int k = 0; k < 3; k++)
				sub->addChild(createCheckMenuItem(G[k], "",
				                                  [=]() {
					                                  int r = mod->selRow;
					                                  return r < (int) mod->rows.size() && mod->rows[r].grow == k;
				                                  },
				                                  [=]() {
					                                  int r = mod->selRow;
					                                  if (r < (int) mod->rows.size()) {
						                                  mod->rows[r].grow = (uint8_t) k;
						                                  mod->rows[r].dropIdx = (uint8_t) k;
						                                  mod->dirty = true;
					                                  }
				                                  }));
		}));
		midifolder::appendMenu(menu);
	}
};

Model* modelTechnoAblauf = createModel<TechnoAblauf, TechnoAblaufWidget>("TechnoAblauf");
