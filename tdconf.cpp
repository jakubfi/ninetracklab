#include <QtGlobal>
#include <QDebug>
#include "tdconf.h"

static int defchmap[9] = { 7, 6, 5, 4, 3, 2, 1, 0, 8 };

// --------------------------------------------------------------------------
TDConf::TDConf()
{
	format = F_NRZ1;
	bpi = 800;
	sampling_speed = 1.0f;
	tape_speed = 50;
	updateFctob();

	qCopy(defchmap, defchmap+9, chmap);
	deskew = bpl * 0.6;
	deskew_auto = true;
	edge_sens = EDGE_RISING;

	glitch_single = true;
	glitch_max = 2;
	glitch_distance = 2;
	realign_margin = 0;
	realign_push = 0;

	unscatter_auto = true;
	qFill(unscatter, unscatter+9, 0);
	unscatter_fixed = 0;

	pe_mark_pulses_min = 40;
	pe_sync_pulses_min = 25;
	pe_bpl_margin = 0.4;
}

// format => fctob => fcpi => BPL => deskew
// bpi ============/       /
// tape_speed ============/
// sampling_speed =======/

// --------------------------------------------------------------------------
void TDConf::updateFctob()
{
	fctob_ratio = format == F_PE ? 2 : 1;
	updateFcpi();
}

// --------------------------------------------------------------------------
void TDConf::updateFcpi()
{
	fcpi = fctob_ratio * bpi;
	updateBPL();
}

// --------------------------------------------------------------------------
void TDConf::updateBPL()
{
	bpl = qRound((1000000.0f * (double) sampling_speed) / ((double) fcpi * (double) tape_speed));
	updateDeskew();
}

// --------------------------------------------------------------------------
void TDConf::updateDeskew()
{
	deskew = 0.2 * bpl;
}

// --------------------------------------------------------------------------
void TDConf::updateBPI()
{
	fcpi = qRound((1000000.0f * (double) sampling_speed) / ((double) bpl * (double) tape_speed));
	bpi = fcpi / fctob_ratio;
}

// --------------------------------------------------------------------------
void TDConf::setFormat(Encoding f)
{
	format = f;
	updateFctob();
}

// --------------------------------------------------------------------------
void TDConf::setBPI(int b)
{
	bpi = b;
	updateFcpi();
}

// --------------------------------------------------------------------------
void TDConf::setSamplingSpeed(int s)
{
	sampling_speed = s;
	updateBPL();
}

// --------------------------------------------------------------------------
void TDConf::setTapeSpeed(int s)
{
	tape_speed = s;
	updateBPL();
}

// --------------------------------------------------------------------------
void TDConf::setBPL(int b)
{
	bpl = b;
	updateBPI();
	updateDeskew();
}

// vim: tabstop=4 shiftwidth=4 autoindent
