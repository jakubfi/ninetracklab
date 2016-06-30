#ifndef DECODERNRZ1_H
#define DECODERNRZ1_H

#include <QtGlobal>

#include "tapedrive.h"
#include "blockstore.h"

class TapeDrive;

// --------------------------------------------------------------------------
class DecoderNRZ1
{
private:
	TapeDrive *td;
	quint16 buf[1024*1024];

public:
	DecoderNRZ1(TapeDrive *td);
	quint16 crc(quint16 *data, int size);

	TapeChunk scan_next_chunk(int start);
	int process(TapeChunk &chunk);

};

#endif

// vim: tabstop=4 shiftwidth=4 autoindent
