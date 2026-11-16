// GENRA TECHNO: engine test without Rack (CI runs it before the build)
#include "../src/tg/Engine.hpp"
#include <cstdio>
#include <cstdlib>
using namespace tg;
static int fails = 0;
#define CHECK(c, ...) do { if (!(c)) { std::printf("FAIL: " __VA_ARGS__); std::printf("\n"); fails++; } } while (0)
int main() {
	std::printf("%d styles\n", numStyles());
	for (int s = 0; s < numStyles(); s++) {
		GenSettings g;
		g.style = s;
		g.key = (s * 5) % 12;
		Pattern p;
		generate(p, g, 1000 + s);
		int kicks = 0, bass = 0, clash = 0;
		for (int i = 0; i < 64; i++) {
			kicks += p.drum[D_KICK][i].vel > 0;
			if (p.mel[0][i].on) { bass++; if (p.drum[D_KICK][i].vel > 100) clash++; }
		}
		CHECK(kicks >= 8, "%s: %d kicks", styles()[s].name, kicks);
		CHECK(bass > 0 || p.len[1] < 64, "%s: no bass", styles()[s].name);
		std::vector<Section> song = planSong(s, 6.f, styles()[s].bpm, 7 + s);
		int bars = songBars(song);
		CHECK(bars >= 64 && bars <= 400, "%s: song %d bars", styles()[s].name, bars);
		std::printf("%-20s kicks %2d bass %2d clash %d  song %3d bars:", styles()[s].name, kicks, bass, clash, bars);
		for (auto& x : song) std::printf(" %s%d%s", PART_NAMES[x.part], x.bars, x.part == P_BUILD ? (std::string("(") + BUILD_NAMES[x.build] + ")").c_str() : "");
		std::printf("  chords %s %s %s %s\n", chordName(p, 0).c_str(), chordName(p, 1).c_str(), chordName(p, 2).c_str(), chordName(p, 3).c_str());
		// play the whole song
		int64_t step = 0;
		int events = 0;
		for (size_t si = 0; si < song.size(); si++)
			for (int b = 0; b < song[si].bars; b++)
				for (int k = 0; k < 16; k++, step++) {
					Ctx c; c.part = song[si].part; c.secBar = b; c.secBars = song[si].bars; c.build = song[si].build;
					c.grow = song[si].grow; c.dropIdx = song[si].dropIdx; c.songStep = step; c.rnd = 3;
					c.nextPart = si + 1 < song.size() ? (int) song[si + 1].part : (int) P_OUTRO;
					StepOut o; resolveStep(p, c, o);
					for (int v = 0; v < DRUMS; v++) events += o.drum[v].vel > 0;
				}
		CHECK(events > bars * 4, "%s: only %d drum events", styles()[s].name, events);
		std::vector<const Pattern*> ps = {&p};
		std::vector<uint8_t> mid = songMidi(ps, song, styles()[s].bpm, 0.6f, true);
		CHECK(mid.size() > 200, "midi too small");
		for (int m = 0; m < NUM_SCALES; m++) {
			Pattern q = p;
			changeScale(q, m);
			for (int i = 0; i < 64; i++)
				if (q.mel[0][i].on) CHECK(inScale(q, q.mel[0][i].pitch[0]) || !inScale(p, p.mel[0][i].pitch[0]), "%s: scale %d", styles()[s].name, m);
		}
		for (int t = 0; t < NUM_TRACKS; t++) mutate(p, g, 5, t, 0.5f);
		generate(p, g, 99, T_LEAD);
	}
	std::printf(fails ? "%d FAILS\n" : "ok\n", fails);
	return fails ? 1 : 0;
}
