#ifndef TDCONF_H
#define TDCONF_H

#include <QtAlgorithms>

enum Encoding { F_NONE, F_PE, F_NRZ1 };
enum EdgeSens { EDGE_NONE, EDGE_RISING, EDGE_FALLING, EDGE_ANY };

// --------------------------------------------------------------------------
class TDConf {
public:
	Encoding format;
	int bpi;				// bits/in
	int fcpi;				// flux changes/in
	int fctob_ratio;		// fcpi to bpi ratio
	int bpl;				// samples
	long sampling_speed;	// MS/s
	int tape_speed;			// in/s
	int chmap[9];			// channel to track mapping: channel = chmap[track]
	int deskew;				// samples
	bool deskew_auto;
	EdgeSens edge_sens;		// edge sensitivity

	bool glitch_single;
	int glitch_max, glitch_distance;
	int realign_margin, realign_push;

	bool unscatter_auto;
	int unscatter[9];
	int unscatter_fixed;

	int pe_mark_pulses_min;
	int pe_sync_pulses_min;
	float pe_bpl_margin;

public:

	TDConf();

	void updateFctob();
	void updateFcpi();
	void updateBPL();
	void updateBPI();
	void updateDeskew();

	void setFormat(Encoding f);
	void setBPI(int b);
	void setBPL(int b);
	void setSamplingSpeed(int s);
	void setTapeSpeed(int s);

	void setCHMap(const int *cm) { qCopy(cm, cm+9, chmap); }
	void setCHMap(int track, int ch) { chmap[track] = ch; }
	void setGlitch(bool single, int max, int distance) { glitch_single = single; glitch_max = max; glitch_distance = distance; }
	void setRealign(int margin, int push) { realign_margin = margin; realign_push = push; }
	void setUnscatter(const int *us) { qCopy(us, us+9, unscatter); }
};

#endif

// vim: tabstop=4 shiftwidth=4 autoindent
