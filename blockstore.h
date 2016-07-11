#ifndef BLOCKSTORE_H
#define BLOCKSTORE_H

#include <QMap>

#include "tdconf.h"

enum EventType {
	C_ROW,
	C_ERROR,
};

enum ChunkType {
	C_NONE,
	C_BLOCK,
	C_MARK,
};

// --------------------------------------------------------------------------
class TapeEvent
{
public:
	EventType type;
	unsigned offset;

	TapeEvent(unsigned offset, EventType type) { this->offset = offset; this->type = type; }
};

// --------------------------------------------------------------------------
class TapeChunk
{
public:
	ChunkType type;				// type
	Encoding format;			// encoding format
	long beg;					// start (samples into tape)
	long end;					// end (samples into tape)
	long len;					// length (samples)

	quint16 *data;				// data
	int bytes;					// data length (bytes)

	int crc_tape, crc_data;		// hparity (on tape, calculated)
	int fixed;					// has the chunk been fixed?
	int hpar_err;				// hparity mismatches
	int hpar_tape, hpar_data;	// crc (on tape, calculated)
	int vpar_err_count;			// number of vparity errors
	int hpar_err_count;			// number of hparity errors

	QList<TapeEvent> events;
	TDConf cfg;

	TapeChunk(long b=0, long e=0);
	~TapeChunk();
	TapeChunk(const TapeChunk &other);
	TapeChunk &operator=(const TapeChunk &other);
};

#endif

// vim: tabstop=4 shiftwidth=4 autoindent
