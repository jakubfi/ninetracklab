#ifndef DECODERPE_H
#define DECODERPE_H

#include "tapedrive.h"

class TapeDrive;
class BlockStore; // TODO: temporary

// --------------------------------------------------------------------------
class DecoderPE
{
private:
	quint16 buf[1024*1024];
	TapeDrive *td;

	int search_mark(int pulse, int pulse_start);
	int search_preamble(int pulse, int pulse_start);
	int find_burst(int *burst_start);
	int get_row(quint16 *data);
	int get_data(BlockStore &bs);
	int get_block(BlockStore &bs);

public:
	DecoderPE(TapeDrive *td);
	int run(BlockStore &bs);

};

#endif

// vim: tabstop=4 shiftwidth=4 autoindent
