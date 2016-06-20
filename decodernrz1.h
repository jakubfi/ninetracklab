#ifndef DECODERNRZ1_H
#define DECODERNRZ1_H

#include <QtGlobal>

#include "tapedrive.h"
#include "blockstore.h"

// --------------------------------------------------------------------------
class DecoderNRZ1
{
private:
	quint16 buf[1024*1024];
	int deskew;
	int edge_sens;
	int bpl4_min, bpl4_max;
	int bpl8_min, bpl8_max;

	int get_block(TapeDrive &td, BlockStore &bs);
public:
	DecoderNRZ1();
	quint16 crc(quint16 *data, int size);
	int run(TapeDrive &td, BlockStore &bs);
	void set_edge_sens(int e) { edge_sens = e; }
	void set_deskew(int d) { deskew = d; }
	void set_cksum_space(int min, int max) { bpl4_min = min, bpl4_max = max; }
	void set_mark_space(int min, int max) { bpl8_min = min, bpl8_max = max; }

};

#endif // DECODERNRZ1_H
