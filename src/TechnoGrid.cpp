// SPDX-License-Identifier: GPL-3.0-only
// GENRA TECHNO GRID: the editor of GENRA TECHNO's pattern. Drums as a raster (8 voices), BASS, STAB, LEAD and ATMO as a
// piano roll; under it velocity, chance and ratchets per step; the red END marker is the track's loop length (click
// in the ruler). Edits go straight into the main module's pattern (UI thread); UNDO / REDO, CLEAR, COPY / PASTE and
// NEW TRACK hand a whole pattern over. REC writes MIDI notes (right-click › MIDI input) at the playing step.
#include "TechnoModule.hpp"
#include "TechnoUI.hpp"
#include "TechnoGridLayout.hpp"
#include <algorithm>
#include <cstring>
#include <cmath>

using namespace tg;
namespace L = technogrid_layout;

// the buttons MIDI-MAP can press (BTN_PARAM + i; only ever append)
static const tui::BtnDef BTNS[] = {
    {L::BTN_TRK0, "DRUMS"},
    {L::BTN_TRK1, "BASS"},
    {L::BTN_TRK2, "STAB"},
    {L::BTN_TRK3, "LEAD"},
    {L::BTN_TRK4, "ATMO"},
    {L::BTN_REC, "REC"},
    {L::BTN_SOLO, "SOLO"},
    {L::BTN_VIEWMODE, "VIEW"},
    {L::BTN_ACC, "ACCENT"},
    {L::BTN_SLIDE, "SLIDE"},
    {L::BTN_SNAP, "SNAP"},
    {L::BTN_FOLLOW, "FOLLOW"},
    {L::BTN_UNDO, "UNDO"},
    {L::BTN_REDO, "REDO"},
    {L::BTN_CLEAR, "CLEAR"},
    {L::BTN_COPY, "COPY"},
    {L::BTN_PASTE, "PASTE"},
    {L::BTN_NEWTRK, "NEW"},
};
static const int NUM_BTNS = (int) (sizeof(BTNS) / sizeof(BTNS[0]));

struct TechnoGrid : Module {
	enum ParamId { BAR_PARAM, ZOOM_PARAM, PITCH_PARAM, VEL_PARAM, NLEN_PARAM, CHANCE_PARAM, RATCH_PARAM, ENUMS(BTN_PARAM, 18), NUM_PARAMS };
	TechnoLink link;
	int track = 1, viewMode = 0;
	bool snap = true, follow = true, rec = false;
	midi::InputQueue midiIn;
	int heldStep[128];
	TechnoGrid() {
		config(NUM_PARAMS, 0, 0, 0);
		static_assert(sizeof(BTNS) / sizeof(BTNS[0]) == 18, "BTN_PARAM count");
		tui::configButtons(this, BTN_PARAM, BTNS, NUM_BTNS);
		configParam(BAR_PARAM, 0.f, 3.f, 0.f, "First bar shown", "", 0.f, 1.f, 1.f);
		paramQuantities[BAR_PARAM]->snapEnabled = true;
		configSwitch(ZOOM_PARAM, 0.f, 2.f, 1.f, "Bars shown", {"1 bar", "2 bars", "4 bars"});
		configParam(PITCH_PARAM, 0.f, 1.f, 0.5f, "Pitch range shown");
		configParam(VEL_PARAM, 1.f, 127.f, 100.f, "Velocity (of new and selected notes)");
		paramQuantities[VEL_PARAM]->snapEnabled = true;
		configParam(NLEN_PARAM, 1.f, 16.f, 1.f, "Length (of new and selected notes)", " 16ths");
		paramQuantities[NLEN_PARAM]->snapEnabled = true;
		configParam(CHANCE_PARAM, 0.f, 100.f, 100.f, "Chance (of the selected note)", " %");
		paramQuantities[CHANCE_PARAM]->snapEnabled = true;
		configParam(RATCH_PARAM, 1.f, 4.f, 1.f, "Ratchet (hits in the 16th)");
		paramQuantities[RATCH_PARAM]->snapEnabled = true;
		// Randomize leaves the selected note alone (these knobs edit it)
		for (int i : {VEL_PARAM, NLEN_PARAM, CHANCE_PARAM, RATCH_PARAM})
			paramQuantities[i]->randomizeEnabled = false;
		link.owner = this;
		for (int& h : heldStep)
			h = -1;
	}
	void process(const ProcessArgs& args) override {
		midi::Message msg;
		while (midiIn.tryPop(&msg, args.frame)) {
			GenraTechno* g = link.main.load();
			if (!g || !rec || !g->running.load())
				continue;
			int st = msg.getStatus(), note = msg.getNote(), vel = msg.getValue();
			if (st == 0x9 && vel > 0) {
				int len = g->pat.trackLen(track);
				int idx = (int) (g->pubSongStep.load() % std::max(1, len));
				if (track == T_DRUMS) {
					int v = -1;
					for (int k = 0; k < DRUMS; k++)
						if (GM_DRUM[k] == note)
							v = k;
					if (v < 0)
						v = ((note - 36) % 8 + 8) % 8;
					int vl = g->pat.voiceLen(v);
					idx = (int) (g->pubSongStep.load() % std::max(1, vl));
					g->pat.drum[v][idx].vel = (uint8_t) vel;
					g->pat.drum[v][idx].chance = 100;
					g->pat.drum[v][idx].ratch = 1;
				}
				else {
					Note& n = g->pat.mel[track - 1][idx];
					n = Note();
					n.on = 1;
					n.pitch[0] = (int8_t) note;
					n.vel = (uint8_t) vel;
					heldStep[note & 127] = idx;
				}
			}
			else if ((st == 0x8 || (st == 0x9 && vel == 0)) && track != T_DRUMS) {
				int idx = heldStep[note & 127];
				if (idx >= 0) {
					int len = g->pat.trackLen(track);
					int now = (int) (g->pubSongStep.load() % std::max(1, len));
					int l = ((now - idx) % len + len) % len;
					g->pat.mel[track - 1][idx].len = (uint8_t) clamp(l, 1, 64);
					heldStep[note & 127] = -1;
				}
			}
		}
	}
	json_t* dataToJson() override {
		json_t* root = json_object();
		link.toJson(root);
		json_object_set_new(root, "track", json_integer(track));
		json_object_set_new(root, "view", json_integer(viewMode));
		json_object_set_new(root, "snap", json_boolean(snap));
		json_object_set_new(root, "follow", json_boolean(follow));
		json_object_set_new(root, "midi", midiIn.toJson());
		return root;
	}
	void dataFromJson(json_t* root) override {
		link.fromJson(root);
		if (json_t* j = json_object_get(root, "track"))
			track = clamp((int) json_integer_value(j), 0, 4);
		if (json_t* j = json_object_get(root, "view"))
			viewMode = clamp((int) json_integer_value(j), 0, 2);
		snap = !json_is_false(json_object_get(root, "snap"));
		follow = !json_is_false(json_object_get(root, "follow"));
		if (json_t* j = json_object_get(root, "midi"))
			midiIn.fromJson(j);
	}
};

static Pattern g_clip;
static int g_clipTrack = -1;

// ================================================================ the editor
struct GridView : widget::OpaqueWidget {
	TechnoGrid* mod = nullptr;
	std::function<void()> pushUndo;
	int selStep = -1, selRow = -1; // selected note (step; drums: voice)
	// geometry (px), set in draw
	float hx = 0, hy = 22, keyW = 40, rollY = 0, rollH = 0, laneY = 0, laneH = 14, colW = 10;
	int first = 0, steps = 32;
	std::vector<int> rows; // pitches (top first) or voices
	bool dragging = false;
	int dragLane = -1;
	Vec dragPos;

	GenraTechno* gm() { return mod ? mod->link.main.load() : nullptr; }

	void layout() {
		hx = keyW;
		rollY = 24.f;
		laneH = 13.f;
		laneY = box.size.y - 3 * (laneH + 3.f) - 10.f;
		rollH = laneY - rollY - 6.f;
		int zoom = mod ? (int) mod->params[TechnoGrid::ZOOM_PARAM].getValue() : 1;
		steps = zoom == 0 ? 16 : (zoom == 1 ? 32 : 64);
		first = mod ? (int) mod->params[TechnoGrid::BAR_PARAM].getValue() * 16 : 0;
		GenraTechno* g = gm();
		if (mod && mod->follow && g && g->running.load()) {
			int len = g->pat.trackLen(mod->track);
			int pos = (int) (g->pubSongStep.load() % std::max(1, len));
			first = (pos / steps) * steps;
		}
		if (first + steps > 64)
			first = std::max(0, 64 - steps);
		colW = (box.size.x - hx - 8.f) / steps;
		rows.clear();
		int t = mod ? mod->track : 1;
		if (t == T_DRUMS) {
			for (int v = 0; v < DRUMS; v++)
				rows.push_back(v);
			return;
		}
		int lo = t == T_BASS ? 24 : 36, hi = t == T_BASS ? 64 : 96;
		std::vector<int> all;
		for (int p = hi; p >= lo; p--) {
			if (g && mod->viewMode == 1 && !inScale(g->pat, p))
				continue;
			if (g && mod->viewMode == 2) {
				bool used = false;
				for (int i = 0; i < STEPS && !used; i++) {
					const Note& n = g->pat.mel[t - 1][i];
					for (int k = 0; k < n.n && n.on; k++)
						used = used || n.pitch[k] == p;
				}
				if (!used)
					continue;
			}
			all.push_back(p);
		}
		int visible = std::max(6, (int) (rollH / 9.f));
		if ((int) all.size() <= visible)
			rows = all;
		else {
			float sc = mod ? 1.f - mod->params[TechnoGrid::PITCH_PARAM].getValue() : 0.5f;
			int start = (int) std::round(sc * (all.size() - visible));
			rows.assign(all.begin() + start, all.begin() + start + visible);
		}
	}
	float rowH() { return rows.empty() ? 10.f : rollH / rows.size(); }

	void draw(const DrawArgs& args) override {
		using namespace tui;
		NVGcontext* vg = args.vg;
		layout();
		GenraTechno* g = gm();
		auto f = bold();
		int t = mod ? mod->track : 1;
		NVGcolor col = TRACK_COL[t];
		// head
		std::string head = TRACK_NAMES[t];
		text(vg, f, 8, 11, head, 13.f, col, NVG_ALIGN_LEFT, 1.f);
		if (!g) {
			text(vg, f, box.size.x / 2, box.size.y / 2, "NO GENRA TECHNO IN THE PATCH", 11.f, DIM, NVG_ALIGN_CENTER, 1.f);
			return;
		}
		Pattern& p = g->pat;
		std::string info = "· " + std::string(typeName(t, g->effectiveType(t)));
		text(vg, f, 8 + text(vg, f, -1000, -1000, head, 13.f, col, NVG_ALIGN_LEFT, 1.f) + 10, 11, info, 9.f, DIM);
		static const char* const VM[3] = {"ALL", "SCALE", "USED"};
		std::string r = string::f("%s · BARS %d–%d · %s %s · LEN %d%s", VM[mod->viewMode], first / 16 + 1, (first + steps) / 16,
		                          KEY_NAMES[p.key % 12], SCALES[p.mode % NUM_SCALES].name, p.trackLen(t), mod->rec ? " · REC" : "");
		text(vg, f, box.size.x - 8, 11, r, 8.5f, mod->rec ? RED : DIM, NVG_ALIGN_RIGHT, 0.6f);
		float rh = rowH();
		// rows
		for (size_t i = 0; i < rows.size(); i++) {
			float y = rollY + i * rh;
			if (t == T_DRUMS) {
				roundRect(vg, 4, y + 0.5f, keyW - 6, rh - 1, 1.f, dark(col, 0.25f));
				text(vg, f, 8, y + rh / 2, DRUM_SHORT[rows[i]], 8.f, TXT);
				roundRect(vg, hx, y, box.size.x - hx - 8, rh, 0, i % 2 ? nvgRGB(0x0d, 0x0e, 0x12) : nvgRGB(0x10, 0x12, 0x16));
			}
			else {
				int pc = ((rows[i] % 12) + 12) % 12;
				bool blk = pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10;
				roundRect(vg, 4, y + 0.3f, blk ? (keyW - 6) * 0.62f : keyW - 6, rh - 0.6f, 0.8f, blk ? nvgRGB(0x0b, 0x0c, 0x0f) : nvgRGB(0xd9, 0xdb, 0xe2));
				if (!blk && rh > 7.f)
					text(vg, f, keyW - 4, y + rh / 2, string::f("%s%d", KEY_NAMES[pc], rows[i] / 12 - 1), std::min(7.f, rh * 0.8f),
					     BG, NVG_ALIGN_RIGHT, 0.f);
				roundRect(vg, hx, y, box.size.x - hx - 8, rh, 0, inScale(p, rows[i]) ? nvgRGB(0x10, 0x12, 0x16) : nvgRGB(0x0a, 0x0b, 0x0e));
			}
		}
		// beat lines
		for (int s = 0; s <= steps; s++) {
			int abs = first + s;
			NVGcolor c = abs % 16 == 0 ? nvgRGB(0x3a, 0x3d, 0x46) : (abs % 4 == 0 ? nvgRGB(0x22, 0x25, 0x2c) : nvgRGB(0x16, 0x18, 0x1d));
			nvgBeginPath(vg);
			nvgMoveTo(vg, hx + s * colW, rollY);
			nvgLineTo(vg, hx + s * colW, rollY + rows.size() * rh);
			nvgStrokeColor(vg, c);
			nvgStrokeWidth(vg, 0.8f);
			nvgStroke(vg);
		}
		// the ruler with the END marker
		int len = p.trackLen(t);
		for (int s = 0; s < steps; s++) {
			int abs = first + s;
			if (abs % 4 == 0)
				text(vg, f, hx + s * colW + 2, rollY - 5, abs % 16 == 0 ? string::f("%d", abs / 16 + 1) : "·", 7.f, DIM);
			if (abs >= len)
				roundRect(vg, hx + s * colW, rollY, colW, rows.size() * rh, 0, nvgRGBA(0xff, 0x4a, 0x4a, 0x14));
		}
		if (len > first && len <= first + steps) {
			float x = hx + (len - first) * colW;
			roundRect(vg, x - 1, rollY - 9, 2, rows.size() * rh + 9, 0.5f, RED);
			text(vg, f, x + 3, rollY - 5, string::f("END %d", len), 7.f, RED);
		}
		// notes / hits
		if (t == T_DRUMS) {
			for (size_t i = 0; i < rows.size(); i++) {
				int v = rows[i];
				for (int s = 0; s < steps; s++) {
					const Hit& h = p.drum[v][first + s];
					if (!h.vel)
						continue;
					float x = hx + s * colW + 1.f, y = rollY + i * rh + 1.f;
					NVGcolor c = mixc(dark(col, 0.5f), col, h.vel / 127.f);
					if (h.chance < 100)
						c.a = 0.55f;
					roundRect(vg, x, y, colW - 2, rh - 2, 1.2f, c);
					if (h.ratch > 1)
						text(vg, f, x + (colW - 2) / 2, y + (rh - 2) / 2, string::f("x%d", h.ratch), 7.f, BG, NVG_ALIGN_CENTER, 0.f);
					if (selStep == first + s && selRow == v)
						frame(vg, x - 0.5f, y - 0.5f, colW - 1, rh - 1, 1.2f, WHITE, 1.2f);
				}
			}
		}
		else {
			const Note* m = p.mel[t - 1];
			float prevX = -1, prevY = -1;
			for (int s = 0; s < 64; s++) {
				const Note& n = m[s];
				if (!n.on)
					continue;
				for (int k = 0; k < std::max(1, (int) n.n); k++) {
					int ri = -1;
					for (size_t i = 0; i < rows.size(); i++)
						if (rows[i] == n.pitch[k])
							ri = (int) i;
					if (ri < 0 || s + n.len <= first || s >= first + steps)
						continue;
					float x = hx + (s - first) * colW + 1.f, y = rollY + ri * rh + 0.8f;
					float w = n.len * colW - 2.f;
					NVGcolor c = mixc(dark(col, 0.5f), col, n.vel / 127.f);
					if (n.chance < 100)
						c.a = 0.55f;
					int r = std::max(1, (int) n.ratch);
					for (int q = 0; q < r; q++)
						roundRect(vg, x + q * w / r, y, w / r - (r > 1 ? 1.f : 0.f), rh - 1.6f, 1.2f, c);
					if (n.acc)
						text(vg, f, x + 2, y + (rh - 1.6f) / 2, "›", 9.f, BG, NVG_ALIGN_LEFT, 0.f);
					if (r > 1)
						text(vg, f, x + w / 2, y - 3, string::f("x%d", r), 6.5f, WHITE, NVG_ALIGN_CENTER, 0.f);
					if (selStep == s)
						frame(vg, x - 0.5f, y - 0.5f, w + 1, rh - 0.6f, 1.2f, WHITE, 1.2f);
					if (k == 0) {
						if (prevX >= 0 && s > 0 && m[s - 1].on && m[s - 1].slide) {
							nvgBeginPath(vg);
							nvgMoveTo(vg, prevX, prevY);
							nvgLineTo(vg, x, y + rh / 2);
							nvgStrokeColor(vg, WHITE);
							nvgStrokeWidth(vg, 0.9f);
							nvgStroke(vg);
						}
						prevX = x + w;
						prevY = y + rh / 2;
					}
				}
			}
		}
		// the playhead
		if (g->running.load()) {
			int pos = (int) (g->pubSongStep.load() % std::max(1, len));
			if (pos >= first && pos < first + steps)
				roundRect(vg, hx + (pos - first + 0.5f) * colW - 0.8f, rollY, 1.6f, rows.size() * rh, 0.5f, WHITE);
		}
		// lanes: velocity, chance, ratchet
		static const char* const LN[3] = {"VEL", "CHANCE", "RATCH"};
		for (int l = 0; l < 3; l++) {
			float y = laneY + l * (laneH + 3.f);
			text(vg, f, hx - 4, y + laneH / 2, LN[l], 7.f, l == 2 ? col : DIM, NVG_ALIGN_RIGHT, 0.4f);
			roundRect(vg, hx, y, box.size.x - hx - 8, laneH, 1.f, nvgRGB(0x0c, 0x0d, 0x10));
			for (int s = 0; s < steps; s++) {
				int abs = first + s;
				int vel = 0, ch = 100, ra = 1;
				bool on = false, sel = false;
				if (t == T_DRUMS) {
					int v = selRow >= 0 ? selRow : D_KICK;
					const Hit& h = p.drum[v][abs];
					on = h.vel > 0;
					vel = h.vel;
					ch = h.chance;
					ra = h.ratch;
					sel = abs == selStep;
				}
				else {
					const Note& n = p.mel[t - 1][abs];
					on = n.on;
					vel = n.vel;
					ch = n.chance;
					ra = n.ratch;
					sel = abs == selStep;
				}
				if (!on)
					continue;
				float x = hx + s * colW + colW * 0.25f, w = colW * 0.5f;
				if (l < 2) {
					float v = l == 0 ? vel / 127.f : ch / 100.f;
					roundRect(vg, x, y + laneH - (laneH - 2) * v - 1, w, (laneH - 2) * v, 0.6f, sel ? WHITE : (l == 0 ? col : DIM));
				}
				else
					text(vg, f, x + w / 2, y + laneH / 2, ra > 1 ? string::f("%d", ra) : "·", 8.f, ra > 1 ? col : DIM, NVG_ALIGN_CENTER, 0.f);
			}
		}
		text(vg, f, box.size.x - 8, box.size.y - 4, "CLICK: ADD / SELECT  ·  RIGHT-CLICK: DELETE  ·  DRAG A NOTE: LENGTH  ·  RULER: END",
		     6.5f, DIM, NVG_ALIGN_RIGHT, 0.4f);
	}

	/** step and row under a point (-1: none) */
	bool hit(Vec p, int* step, int* row) {
		if (p.x < hx || p.y < rollY || rows.empty())
			return false;
		*step = first + (int) ((p.x - hx) / colW);
		*row = (int) ((p.y - rollY) / rowH());
		return *step >= 0 && *step < 64 && *step < first + steps && *row >= 0 && *row < (int) rows.size();
	}
	/** the melodic note covering (step, pitch): its start step, else -1 */
	int noteAt(int t, int step, int pitch) {
		GenraTechno* g = gm();
		for (int s = step; s >= 0 && s >= step - 64; s--) {
			const Note& n = g->pat.mel[t - 1][s];
			if (!n.on)
				continue;
			if (s + n.len > step)
				for (int k = 0; k < std::max(1, (int) n.n); k++)
					if (n.pitch[k] == pitch)
						return s;
			if (s < step)
				break;
		}
		return -1;
	}
	void selectInto() {
		// the knobs show the selected note
		GenraTechno* g = gm();
		if (!g || selStep < 0)
			return;
		int t = mod->track;
		if (t == T_DRUMS) {
			if (selRow < 0)
				return;
			const Hit& h = g->pat.drum[selRow][selStep];
			mod->params[TechnoGrid::VEL_PARAM].setValue(h.vel ? h.vel : 100);
			mod->params[TechnoGrid::CHANCE_PARAM].setValue(h.chance);
			mod->params[TechnoGrid::RATCH_PARAM].setValue(h.ratch);
		}
		else {
			const Note& n = g->pat.mel[t - 1][selStep];
			mod->params[TechnoGrid::VEL_PARAM].setValue(n.vel);
			mod->params[TechnoGrid::NLEN_PARAM].setValue(std::min(16, (int) n.len));
			mod->params[TechnoGrid::CHANCE_PARAM].setValue(n.chance);
			mod->params[TechnoGrid::RATCH_PARAM].setValue(n.ratch);
		}
	}
	std::function<void()> knobsTaken;

	void onButton(const ButtonEvent& e) override {
		GenraTechno* g = gm();
		if (!g || e.action != GLFW_PRESS) {
			OpaqueWidget::onButton(e);
			return;
		}
		e.consume(this);
		int t = mod->track;
		// the ruler: the END marker
		if (e.pos.y < rollY && e.pos.y > rollY - 12 && e.pos.x >= hx) {
			int s = first + (int) ((e.pos.x - hx) / colW) + 1;
			pushUndo();
			g->pat.len[t] = (uint8_t) clamp(s, 1, 64);
			g->params[GenraTechno::LEN_PARAM + t].setValue(g->pat.len[t]);
			return;
		}
		// the lanes
		if (e.pos.y >= laneY) {
			int l = (int) ((e.pos.y - laneY) / (laneH + 3.f));
			int s = first + (int) ((e.pos.x - hx) / colW);
			if (l == 2 && s >= first && s < first + steps) {
				pushUndo();
				if (t == T_DRUMS) {
					Hit& h = g->pat.drum[selRow >= 0 ? selRow : D_KICK][s];
					if (h.vel)
						h.ratch = (uint8_t) (h.ratch % 4 + 1);
				}
				else {
					Note& n = g->pat.mel[t - 1][s];
					if (n.on)
						n.ratch = (uint8_t) (n.ratch % 4 + 1);
				}
				return;
			}
			if (l < 2) {
				pushUndo();
				dragging = true;
				dragLane = l;
				dragPos = e.pos;
				laneSet(e.pos);
			}
			return;
		}
		int s, r;
		if (!hit(e.pos, &s, &r))
			return;
		int vel = (int) mod->params[TechnoGrid::VEL_PARAM].getValue();
		if (t == T_DRUMS) {
			Hit& h = g->pat.drum[rows[r]][s];
			pushUndo();
			if (e.button == GLFW_MOUSE_BUTTON_RIGHT || (h.vel && selStep == s && selRow == rows[r])) {
				h = Hit();
				selStep = -1;
			}
			else {
				if (!h.vel) {
					h.vel = (uint8_t) vel;
					h.chance = 100;
					h.ratch = 1;
				}
				selStep = s;
				selRow = rows[r];
				selectInto();
			}
			if (knobsTaken)
				knobsTaken();
			return;
		}
		int pitch = rows[r];
		int at = noteAt(t, s, pitch);
		Note* m = g->pat.mel[t - 1];
		pushUndo();
		if (e.button == GLFW_MOUSE_BUTTON_RIGHT) {
			if (at >= 0) {
				Note& n = m[at];
				// poly: take out the one voice; mono: the note
				int k = 0;
				for (; k < n.n; k++)
					if (n.pitch[k] == pitch)
						break;
				if (n.n > 1 && k < n.n) {
					for (int q = k; q + 1 < n.n; q++)
						n.pitch[q] = n.pitch[q + 1];
					n.n--;
				}
				else
					n.on = 0;
			}
			selStep = -1;
			if (knobsTaken)
				knobsTaken();
			return;
		}
		if (at >= 0) {
			selStep = at;
			selectInto();
			dragging = true;
			dragLane = -1;
			dragPos = e.pos;
			if (knobsTaken)
				knobsTaken();
			return;
		}
		// a new note (snapped to the scale when SNAP is on); STAB and ATMO add a voice to a chord on the step
		if (mod->snap && !inScale(g->pat, pitch))
			return;
		Note& n = m[s];
		bool poly = t == T_STAB || t == T_ATMO;
		if (n.on && poly && n.n < VOICES) {
			n.pitch[n.n++] = (int8_t) pitch;
		}
		else {
			n = Note();
			n.on = 1;
			n.pitch[0] = (int8_t) pitch;
			n.n = 1;
			n.vel = (uint8_t) vel;
			n.len = (uint8_t) mod->params[TechnoGrid::NLEN_PARAM].getValue();
		}
		selStep = s;
		selectInto();
		if (knobsTaken)
			knobsTaken();
	}
	void laneSet(Vec p) {
		GenraTechno* g = gm();
		if (!g)
			return;
		int t = mod->track;
		int s = first + (int) ((p.x - hx) / colW);
		if (s < first || s >= first + steps)
			return;
		float y = laneY + dragLane * (laneH + 3.f);
		float v = clamp(1.f - (p.y - y) / laneH, 0.f, 1.f);
		if (t == T_DRUMS) {
			Hit& h = g->pat.drum[selRow >= 0 ? selRow : D_KICK][s];
			if (!h.vel)
				return;
			if (dragLane == 0)
				h.vel = (uint8_t) clamp((int) (v * 127), 1, 127);
			else
				h.chance = (uint8_t) clamp((int) (v * 100), 0, 100);
		}
		else {
			Note& n = g->pat.mel[t - 1][s];
			if (!n.on)
				return;
			if (dragLane == 0)
				n.vel = (uint8_t) clamp((int) (v * 127), 1, 127);
			else
				n.chance = (uint8_t) clamp((int) (v * 100), 0, 100);
		}
	}
	void onDragStart(const DragStartEvent& e) override { e.consume(this); }
	void onDragMove(const DragMoveEvent& e) override {
		if (!dragging)
			return;
		dragPos = dragPos.plus(e.mouseDelta.div(getAbsoluteZoom()));
		GenraTechno* g = gm();
		if (!g)
			return;
		if (dragLane >= 0) {
			laneSet(dragPos);
			return;
		}
		// the length of the selected note
		int t = mod->track;
		if (t == T_DRUMS || selStep < 0)
			return;
		int end = first + (int) std::ceil((dragPos.x - hx) / colW);
		Note& n = g->pat.mel[t - 1][selStep];
		n.len = (uint8_t) clamp(end - selStep, 1, 64);
		mod->params[TechnoGrid::NLEN_PARAM].setValue(std::min(16, (int) n.len));
		if (knobsTaken)
			knobsTaken();
	}
	void onDragEnd(const DragEndEvent& e) override { dragging = false; }
};

// ================================================================ the panel
struct TechnoGridWidget : ModuleWidget {
	TechnoGrid* mod = nullptr;
	GridView* view = nullptr;
	float last[TechnoGrid::NUM_PARAMS] = {};
	bool seen = false;
	// edits by hand: for a moment after one, what changes in the pattern goes to ABLAUF's pad (userEdit)
	float editT = 0.f;
	int editPad = -1;
	Pattern editSeen;
	float clearArm = 0.f, lockedT = 0.f; // CLEAR: press twice; NEW TRACK on a locked track: "LOCKED"

	GenraTechno* gm() { return mod ? mod->link.main.load() : nullptr; }
	/** One UNDO step (shared with GENRA TECHNO's UNDO), and watch the edit that follows. */
	void pushUndo() {
		GenraTechno* g = gm();
		if (!g)
			return;
		g->pushUndo();
		touched();
	}
	void touched() {
		GenraTechno* g = gm();
		if (!g)
			return;
		if (editT <= 0.f) {
			editPad = g->pubPad.load();
			std::memcpy((void*) &editSeen, (const void*) &g->pat, sizeof(Pattern));
		}
		editT = 2.f;
	}

	bool btnsMapped = false;
	TechnoGridWidget(TechnoGrid* module) {
		setModule(module);
		mod = module;
		setPanel(createPanel(asset::plugin(pluginInstance, "res/TechnoGrid.svg")));
		using namespace tui;
		addParam(createParamCentered<GSmallKnob>(mm(L::K_BAR), module, TechnoGrid::BAR_PARAM));
		addParam(createParamCentered<GSmallKnob>(mm(L::K_ZOOM), module, TechnoGrid::ZOOM_PARAM));
		addParam(createParamCentered<GSmallKnob>(mm(L::K_PITCH), module, TechnoGrid::PITCH_PARAM));
		addParam(createParamCentered<GSmallKnob>(mm(L::K_VEL), module, TechnoGrid::VEL_PARAM));
		addParam(createParamCentered<GSmallKnob>(mm(L::K_NLEN), module, TechnoGrid::NLEN_PARAM));
		addParam(createParamCentered<GSmallKnob>(mm(L::K_CHANCE), module, TechnoGrid::CHANCE_PARAM));
		addParam(createParamCentered<GSmallKnob>(mm(L::K_RATCH), module, TechnoGrid::RATCH_PARAM));
		view = place<GridView>(this, L::DISP_GRID);
		view->mod = module;
		view->pushUndo = [=]() { pushUndo(); };
		view->knobsTaken = [=]() {
			if (mod)
				for (int i = 0; i < TechnoGrid::NUM_PARAMS; i++)
					last[i] = mod->params[i].getValue();
		};
		static const float* const TB[5] = {L::BTN_TRK0, L::BTN_TRK1, L::BTN_TRK2, L::BTN_TRK3, L::BTN_TRK4};
		for (int t = 0; t < NUM_TRACKS; t++) {
			TButton* b = place<TButton>(this, TB[t]);
			b->label = TRACK_NAMES[t];
			b->fontSize = 7.5f;
			b->colOf = [=]() { return mod && mod->track == t ? dark(TRACK_COL[t], 0.75f) : BTN; };
			b->act = [=]() {
				if (mod) {
					mod->track = t;
					view->selStep = -1;
				}
			};
		}
		TButton* rec = place<TButton>(this, L::BTN_REC);
		rec->label = "REC";
		rec->colOf = [=]() { return mod && mod->rec ? RED : dark(RED, 0.5f); };
		rec->textCol = WHITE;
		rec->act = [=]() {
			if (mod) {
				if (!mod->rec)
					pushUndo();
				mod->rec = !mod->rec;
			}
		};
		TButton* solo = place<TButton>(this, L::BTN_SOLO);
		solo->label = "SOLO";
		solo->lit = [=]() { return gm() && mod && gm()->solo.load() == mod->track; };
		solo->act = [=]() {
			if (GenraTechno* g = gm())
				g->solo = g->solo.load() == mod->track ? -1 : mod->track;
		};
		TButton* vm = place<TButton>(this, L::BTN_VIEWMODE);
		vm->labelOf = [=]() {
			static const char* const VM[3] = {"VIEW: ALL", "VIEW: SCALE", "VIEW: USED"};
			return std::string(VM[mod ? mod->viewMode : 0]);
		};
		vm->act = [=]() {
			if (mod)
				mod->viewMode = (mod->viewMode + 1) % 3;
		};
		TButton* acc = place<TButton>(this, L::BTN_ACC);
		acc->label = "ACCENT";
		acc->lit = [=]() {
			GenraTechno* g = gm();
			return g && mod->track > 0 && view->selStep >= 0 && g->pat.mel[mod->track - 1][view->selStep].acc;
		};
		acc->act = [=]() {
			GenraTechno* g = gm();
			if (g && mod->track > 0 && view->selStep >= 0) {
				pushUndo();
				Note& n = g->pat.mel[mod->track - 1][view->selStep];
				n.acc = !n.acc;
			}
		};
		TButton* sl = place<TButton>(this, L::BTN_SLIDE);
		sl->label = "SLIDE";
		sl->lit = [=]() {
			GenraTechno* g = gm();
			return g && mod->track > 0 && view->selStep >= 0 && g->pat.mel[mod->track - 1][view->selStep].slide;
		};
		sl->act = [=]() {
			GenraTechno* g = gm();
			if (g && mod->track > 0 && view->selStep >= 0) {
				pushUndo();
				Note& n = g->pat.mel[mod->track - 1][view->selStep];
				n.slide = !n.slide;
			}
		};
		TButton* sn = place<TButton>(this, L::BTN_SNAP);
		sn->label = "SNAP";
		sn->lit = [=]() { return mod && mod->snap; };
		sn->act = [=]() {
			if (mod)
				mod->snap = !mod->snap;
		};
		TButton* fo = place<TButton>(this, L::BTN_FOLLOW);
		fo->label = "FOLLOW";
		fo->lit = [=]() { return mod && mod->follow; };
		fo->act = [=]() {
			if (mod)
				mod->follow = !mod->follow;
		};
		TButton* un = place<TButton>(this, L::BTN_UNDO);
		un->label = "UNDO";
		un->act = [=]() {
			if (GenraTechno* g = gm()) {
				g->undoStep();
				view->selStep = -1;
			}
		};
		TButton* re = place<TButton>(this, L::BTN_REDO);
		re->label = "REDO";
		re->act = [=]() {
			if (GenraTechno* g = gm()) {
				g->redoStep();
				view->selStep = -1;
			}
		};
		TButton* cl = place<TButton>(this, L::BTN_CLEAR);
		cl->labelOf = [=]() { return std::string(clearArm > 0.f ? "SURE?" : "CLEAR"); };
		cl->lit = [=]() { return clearArm > 0.f; };
		cl->act = [=]() {
			GenraTechno* g = gm();
			if (g && clearArm <= 0.f) {
				clearArm = 4.f; // press again to clear the track
				return;
			}
			clearArm = 0.f;
			if (g) {
				pushUndo();
				Pattern p = g->pat;
				p.clearTrack(mod->track);
				g->post(p, true);
				g->pat = p;
				view->selStep = -1;
			}
		};
		TButton* co = place<TButton>(this, L::BTN_COPY);
		co->label = "COPY";
		co->act = [=]() {
			if (GenraTechno* g = gm()) {
				g_clip = g->pat;
				g_clipTrack = mod->track;
			}
		};
		TButton* pa = place<TButton>(this, L::BTN_PASTE);
		pa->label = "PASTE";
		pa->act = [=]() {
			GenraTechno* g = gm();
			int t = mod->track;
			if (!g || g_clipTrack < 0 || ((t == T_DRUMS) != (g_clipTrack == T_DRUMS)))
				return;
			pushUndo();
			Pattern p = g->pat;
			if (t == T_DRUMS) {
				std::memcpy(p.drum, g_clip.drum, sizeof p.drum);
				std::memcpy(p.vlen, g_clip.vlen, sizeof p.vlen);
			}
			else
				std::memcpy(p.mel[t - 1], g_clip.mel[g_clipTrack - 1], sizeof p.mel[0]);
			p.len[t] = g_clip.len[g_clipTrack];
			g->post(p, true);
			g->pat = p;
		};
		TButton* nt = place<TButton>(this, L::BTN_NEWTRK);
		nt->labelOf = [=]() { return std::string(lockedT > 0.f ? "LOCKED" : "NEW TRACK"); };
		nt->col = NEWCOL;
		nt->textCol = WHITE;
		nt->act = [=]() {
			if (GenraTechno* g = gm()) {
				if (g->lock[mod->track]) {
					lockedT = 1.5f; // a locked track is never made new
					return;
				}
				g->newReq |= 1 << mod->track; // (GENRA TECHNO makes it, with one UNDO step)
			}
		};
		Drawn* ln = place<Drawn>(this, L::A_LINK);
		ln->paintLit = [=](const DrawArgs& a) {
			bool on = gm() != nullptr;
			nvgBeginPath(a.vg);
			nvgCircle(a.vg, 6, ln->box.size.y / 2, 3.5f);
			nvgFillColor(a.vg, on ? (mod && mod->link.fellBack ? nvgRGB(0xff, 0xa0, 0x30) : GREEN) : nvgRGB(0x44, 0x46, 0x50));
			nvgFill(a.vg);
			text(a.vg, bold(), 14, ln->box.size.y / 2, on ? "LINK › GENRA TECHNO" : "NO LINK", 8.f, on ? TXT : DIM);
		};
	}

	void step() override {
		ModuleWidget::step();
		if (!btnsMapped) { // the buttons for MIDI-MAP (the panel is built by now)
			tui::mapButtons(this, module, TechnoGrid::BTN_PARAM, BTNS, NUM_BTNS);
			btnsMapped = true;
		}
		if (!mod)
			return;
		float dt = APP->window->getLastFrameDuration();
		mod->link.uiStep(dt);
		GenraTechno* g = gm();
		if (clearArm > 0.f)
			clearArm -= dt;
		if (lockedT > 0.f)
			lockedT -= dt;
		// an edit by hand: the changed pattern to ABLAUF's pad (only while the same pad plays)
		if (editT > 0.f) {
			editT -= dt;
			if (!g || g->pubPad.load() != editPad)
				editT = 0.f;
			else if (std::memcmp((const void*) &g->pat, (const void*) &editSeen, sizeof(Pattern))) {
				std::memcpy((void*) &editSeen, (const void*) &g->pat, sizeof(Pattern));
				g->userEdit(g->pat);
			}
		}
		// a turned knob changes the selected note
		for (int i = TechnoGrid::VEL_PARAM; i <= TechnoGrid::RATCH_PARAM; i++) {
			float v = mod->params[i].getValue();
			if (seen && v != last[i] && g && view->selStep >= 0) {
				touched();
				int t = mod->track, s = view->selStep;
				if (t == T_DRUMS && view->selRow >= 0) {
					Hit& h = g->pat.drum[view->selRow][s];
					if (h.vel) {
						if (i == TechnoGrid::VEL_PARAM)
							h.vel = (uint8_t) v;
						if (i == TechnoGrid::CHANCE_PARAM)
							h.chance = (uint8_t) v;
						if (i == TechnoGrid::RATCH_PARAM)
							h.ratch = (uint8_t) v;
					}
				}
				else if (t > 0) {
					Note& n = g->pat.mel[t - 1][s];
					if (n.on) {
						if (i == TechnoGrid::VEL_PARAM)
							n.vel = (uint8_t) v;
						if (i == TechnoGrid::NLEN_PARAM)
							n.len = (uint8_t) v;
						if (i == TechnoGrid::CHANCE_PARAM)
							n.chance = (uint8_t) v;
						if (i == TechnoGrid::RATCH_PARAM)
							n.ratch = (uint8_t) v;
					}
				}
			}
			last[i] = v;
		}
		seen = true;
	}

	void appendContextMenu(Menu* menu) override {
		if (!mod)
			return;
		mod->link.appendMenu(menu);
		menu->addChild(new MenuSeparator);
		menu->addChild(createMenuLabel("MIDI input (REC)"));
		appendMidiMenu(menu, &mod->midiIn);
	}
};

Model* modelTechnoGrid = createModel<TechnoGrid, TechnoGridWidget>("TechnoGrid");
