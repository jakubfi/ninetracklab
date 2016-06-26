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
	long beg;
	long end;
	long len;
	ChunkType type;
	Encoding format;
	int bytes;
	quint8 *data;
	quint8 *vparity;
	int b_crc, b_hparity;
	int d_crc, d_hparity;
	int vparity_errors;
	QList<TapeEvent> events;
	TDConf cfg;

	TapeChunk();
	TapeChunk(long b, long e);
	~TapeChunk();
	TapeChunk(const TapeChunk &other);
	TapeChunk &operator=(const TapeChunk &other);
};

#endif // BLOCKSTORE_H
