#ifndef DECODERPE_H
#define DECODERPE_H

#include "tapedrive.h"

// --------------------------------------------------------------------------
class DecoderPE
{
private:
	quint16 buf[1024*1024];
	int deskew;
	int edge_sens;
	int bpl_min, bpl_max;
	int sync_pulses_min, mark_pulses_min;
	int bpl2_min, bpl2_max;

	int search_mark(int pulse, int pulse_start);
	int search_preamble(int pulse, int pulse_start);
	int find_burst(TapeDrive &td, int *burst_start);
	int get_row(TapeDrive &td, quint16 *data);
	int get_data(TapeDrive &td, BlockStore &bs);
	int get_block(TapeDrive &td, BlockStore &bs);

public:
	DecoderPE();
	int run(TapeDrive &td, BlockStore &bs);
	void set_edge_sens(int e) { edge_sens = e; }
	void set_deskew(int d) { deskew = d; }
	void set_bpl(int min, int max) { bpl_min = min; bpl_max = max; }
	void set_sync_pulses(int pulses) { sync_pulses_min = pulses; }
	void set_mark_pulses(int pulses) { mark_pulses_min = pulses; }
};

#endif // DECODERPE_H
