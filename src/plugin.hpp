// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <rack.hpp>

using namespace rack;

extern Plugin* pluginInstance;
extern Model* modelGenraTechno;
extern Model* modelTechnoGrid;
extern Model* modelTechnoAblauf;
extern Model* modelTechnoMod;
extern Model* modelTechnoKugel;

// the GENRA knobs: dark, the same in every module of the family (res/knob/*.svg)
struct GenraKnob : RoundKnob {
	void load(const char* n);
};
struct GLargeKnob : GenraKnob {
	GLargeKnob() { load("large"); }
};
struct GKnob : GenraKnob {
	GKnob() { load("medium"); }
};
struct GSmallKnob : GenraKnob {
	GSmallKnob() { load("small"); }
};
