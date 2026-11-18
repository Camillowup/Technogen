// SPDX-License-Identifier: GPL-3.0-only
// GENRA TECHNO: the look shared by its modules (dark GENRA look, docs/DESIGN.md of the GENRA repo): colours, fonts,
// buttons and dropdown fields (click opens the list, the mouse wheel steps through it).
#pragma once
#include "plugin.hpp"
#include <functional>

namespace tui {

static const NVGcolor BG = nvgRGB(0x17, 0x18, 0x1d), PLATE = nvgRGB(0x21, 0x23, 0x29), EDGE = nvgRGB(0x34, 0x36, 0x3f),
                      TXT = nvgRGB(0xd9, 0xdb, 0xe2), DIM = nvgRGB(0x8a, 0x8d, 0x99), DISP = nvgRGB(0x07, 0x08, 0x0a),
                      BTN = nvgRGB(0x2c, 0x2e, 0x36), RED = nvgRGB(0xff, 0x4a, 0x4a), WHITE = nvgRGB(0xf2, 0xf3, 0xf7),
                      YEL = nvgRGB(0xe3, 0xe3, 0x6a), GREEN = nvgRGB(0x5d, 0xff, 0x8a);
// the meaning of a key's colour (the same in all four modules): orange makes something new, green plays, blue is the
// mode that is chosen
static const NVGcolor NEWCOL = nvgRGB(0xff, 0x8c, 0x2e), PLAYCOL = nvgRGB(0x2f, 0xb8, 0x66),
                      MODECOL = nvgLerpRGBA(PLATE, nvgRGB(0x3d, 0xc9, 0xff), 0.55f);
// the tracks: drums, bass, stab, lead, atmo
static const NVGcolor TRACK_COL[5] = {nvgRGB(0x7b, 0x6c, 0xff), nvgRGB(0xb8, 0xf2, 0x3a), nvgRGB(0x3d, 0xc9, 0xff),
                                      nvgRGB(0x5f, 0xe0, 0xc8), nvgRGB(0xe0, 0x77, 0x9e)};
// the parts: intro, groove, build, drop, break, outro
static const NVGcolor PART_COL[6] = {nvgRGB(0x8a, 0x93, 0xa6), nvgRGB(0x7b, 0x6c, 0xff), nvgRGB(0xe3, 0xe3, 0x6a),
                                     nvgRGB(0xff, 0x4a, 0x4a), nvgRGB(0x3d, 0xc9, 0xff), nvgRGB(0x8a, 0x93, 0xa6)};
// the lanes of MOD
static const NVGcolor LANE_COL[4] = {nvgRGB(0xff, 0x6b, 0x6b), nvgRGB(0x3d, 0xc9, 0xff), nvgRGB(0xb8, 0xf2, 0x3a),
                                     nvgRGB(0xe0, 0x77, 0x9e)};

inline NVGcolor mixc(NVGcolor a, NVGcolor b, float t) { return nvgLerpRGBA(a, b, t); }
inline NVGcolor dark(NVGcolor c, float a = 0.35f) { return nvgLerpRGBA(PLATE, c, a); }

inline Vec mm(const float* p) { return mm2px(Vec(p[0], p[1])); }
inline Rect mmRect(const float* p) { return Rect(mm2px(Vec(p[0], p[1])), mm2px(Vec(p[2], p[3]))); }

inline std::shared_ptr<window::Font> bold() { return APP->window->loadFont(asset::system("res/fonts/Nunito-Bold.ttf")); }
inline std::shared_ptr<window::Font> mono() {
	return APP->window->loadFont(asset::system("res/fonts/ShareTechMono-Regular.ttf"));
}

/** Text; align NVG_ALIGN_LEFT / CENTER / RIGHT, vertically centred on y. Returns the width. */
inline float text(NVGcontext* vg, const std::shared_ptr<window::Font>& f, float x, float y, const std::string& s, float size,
                  NVGcolor c, int align = NVG_ALIGN_LEFT, float spacing = 0.5f) {
	if (!f || f->handle < 0 || s.empty())
		return 0.f;
	nvgFontFaceId(vg, f->handle);
	nvgFontSize(vg, size);
	nvgTextLetterSpacing(vg, spacing);
	nvgTextAlign(vg, align | NVG_ALIGN_MIDDLE);
	nvgFillColor(vg, c);
	float b[4];
	nvgTextBounds(vg, 0, 0, s.c_str(), NULL, b);
	nvgText(vg, x, y, s.c_str(), NULL);
	return b[2] - b[0];
}
/** Text that never gets wider than maxW (smaller if it has to). */
inline void textFit(NVGcontext* vg, const std::shared_ptr<window::Font>& f, float x, float y, const std::string& s, float size,
                    float maxW, NVGcolor c, int align = NVG_ALIGN_LEFT, float spacing = 0.5f) {
	if (!f || f->handle < 0 || s.empty())
		return;
	nvgFontFaceId(vg, f->handle);
	nvgFontSize(vg, size);
	nvgTextLetterSpacing(vg, spacing);
	float w = nvgTextBounds(vg, 0, 0, s.c_str(), NULL, NULL);
	if (w > maxW && maxW > 4.f)
		size = std::max(5.f, size * maxW / w);
	text(vg, f, x, y, s, size, c, align, spacing);
}
inline void roundRect(NVGcontext* vg, float x, float y, float w, float h, float r, NVGcolor c) {
	nvgBeginPath(vg);
	nvgRoundedRect(vg, x, y, w, h, r);
	nvgFillColor(vg, c);
	nvgFill(vg);
}
inline void frame(NVGcontext* vg, float x, float y, float w, float h, float r, NVGcolor c, float sw = 1.f) {
	nvgBeginPath(vg);
	nvgRoundedRect(vg, x, y, w, h, r);
	nvgStrokeColor(vg, c);
	nvgStrokeWidth(vg, sw);
	nvgStroke(vg);
}

/** A button: the label in it (shrinks to fit); `lit` lights a toggle, `colOf` lets the colour change. */
struct TButton : widget::OpaqueWidget {
	std::string label;
	std::function<std::string()> labelOf;
	NVGcolor col = BTN, textCol = TXT;
	std::function<NVGcolor()> colOf;
	std::function<bool()> lit;
	std::function<bool()> enabled; // false: greyed out, a click does nothing
	std::function<void()> act, release;
	std::function<void(ui::Menu*)> rightMenu;
	float fontSize = 9.f;
	bool down = false;
	void draw(const DrawArgs& args) override {
		NVGcolor c = colOf ? colOf() : col;
		bool off = enabled && !enabled();
		bool on = !off && lit && lit();
		if (on)
			c = mixc(c, WHITE, 0.18f);
		if (down)
			c = mixc(c, nvgRGB(0, 0, 0), 0.25f);
		roundRect(args.vg, 0, 0, box.size.x, box.size.y, 2.6f, c);
		if (on)
			frame(args.vg, 0.8f, 0.8f, box.size.x - 1.6f, box.size.y - 1.6f, 2.2f, WHITE, 1.3f);
		textFit(args.vg, bold(), box.size.x / 2, box.size.y / 2 + 0.5f, labelOf ? labelOf() : label, fontSize,
		        box.size.x - 6.f, off ? mixc(textCol, c, 0.65f) : textCol, NVG_ALIGN_CENTER);
	}
	void onButton(const ButtonEvent& e) override {
		if (e.button == GLFW_MOUSE_BUTTON_LEFT && e.action == GLFW_PRESS) {
			if (enabled && !enabled()) {
				e.consume(this);
				return;
			}
			down = true;
			if (act)
				act();
			e.consume(this);
		}
		if (e.button == GLFW_MOUSE_BUTTON_RIGHT && e.action == GLFW_PRESS && rightMenu) {
			rightMenu(createMenu());
			e.consume(this);
		}
	}
	void onDragEnd(const DragEndEvent& e) override {
		down = false;
		if (release)
			release();
	}
};

/** A dropdown field: click opens the list, the mouse wheel steps through it. */
struct TField : widget::OpaqueWidget {
	std::string prefix;
	NVGcolor col = TXT;
	bool fit = false; // shrink a long value to the field (else it is cut)
	std::function<std::string()> value;
	std::function<int()> count, current;
	std::function<std::string(int)> item;
	std::function<void(int)> choose;
	std::function<bool(int)> separatorBefore;
	void draw(const DrawArgs& args) override {
		std::string v = value ? value() : "";
		nvgSave(args.vg);
		nvgIntersectScissor(args.vg, 0, 0, box.size.x - 10.f, box.size.y);
		float x = 4.5f;
		if (!prefix.empty())
			x += text(args.vg, bold(), x, box.size.y / 2 + 0.5f, prefix, 8.f, mixc(col, BG, 0.35f)) + 5.f;
		if (fit)
			textFit(args.vg, bold(), x, box.size.y / 2 + 0.5f, v, 8.5f, box.size.x - 12.f - x, col);
		else
			text(args.vg, bold(), x, box.size.y / 2 + 0.5f, v, 8.5f, col);
		nvgRestore(args.vg);
		float ax = box.size.x - 6.f, ay = box.size.y / 2;
		nvgBeginPath(args.vg);
		nvgMoveTo(args.vg, ax - 3.f, ay - 1.5f);
		nvgLineTo(args.vg, ax + 3.f, ay - 1.5f);
		nvgLineTo(args.vg, ax, ay + 2.f);
		nvgClosePath(args.vg);
		nvgFillColor(args.vg, col);
		nvgFill(args.vg);
	}
	void onButton(const ButtonEvent& e) override {
		if (e.button == GLFW_MOUSE_BUTTON_LEFT && e.action == GLFW_PRESS && count) {
			ui::Menu* menu = createMenu();
			int n = count(), cur = current ? current() : -1;
			for (int i = 0; i < n; i++) {
				if (separatorBefore && separatorBefore(i))
					menu->addChild(new ui::MenuSeparator);
				menu->addChild(createCheckMenuItem(item(i), "", [=]() { return cur == i; }, [=]() { choose(i); }));
			}
			e.consume(this);
		}
	}
	void onHoverScroll(const HoverScrollEvent& e) override {
		if (!count || !current)
			return;
		int n = count();
		int d = e.scrollDelta.y > 0 ? -1 : (e.scrollDelta.y < 0 ? 1 : 0);
		if (d && n > 0)
			choose(clamp(current() + d, 0, n - 1));
		e.consume(this);
	}
};

template <class W>
W* place(ModuleWidget* mw, const float* r) {
	W* w = new W;
	w->box = mmRect(r);
	mw->addChild(w);
	return w;
}

/** A widget that draws with a function (displays). */
struct Drawn : widget::Widget {
	std::function<void(const DrawArgs&)> paint;
	std::function<void(const DrawArgs&)> paintLit; // layer 1 (lit in the dark room)
	void draw(const DrawArgs& args) override {
		if (paint)
			paint(args);
	}
	void drawLayer(const DrawArgs& args, int layer) override {
		if (layer == 1 && paintLit)
			paintLit(args);
		Widget::drawLayer(args, layer);
	}
};

} // namespace tui
