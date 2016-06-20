#ifndef BLOCKSTORE_H
#define BLOCKSTORE_H

#include <QMap>
#include <QAbstractTableModel>

enum chunk_types {
	C_NONE,
	C_GAP,
	C_BLOCK,
	C_MARK,
	C_ROW,
	C_PREAMBLE,
	C_POSTAMBLE,
	C_CRC,
	C_VPARITY,
	C_HPARITY,
	C_TIME,
	C_EVPAR,
};

// --------------------------------------------------------------------------
class TapeEvent
{
public:
	int type;
	unsigned offset;
	QString note;

	TapeEvent(unsigned offset, unsigned type, QString n) { this->offset = offset; this->type = type; this->note = n; }
};

// --------------------------------------------------------------------------
class TapeChunk
{
public:
	int type;
	int format;
	unsigned len;
	int bytes;
	quint8 *data;
	quint8 *vparity;
	int b_crc, b_hparity;
	int d_crc, d_hparity;
	QList<TapeEvent> events;

	TapeChunk(unsigned len, QList<TapeEvent> &e); // gap
	TapeChunk(int format, unsigned len, QList<TapeEvent> &e); // mark
	TapeChunk(int format, unsigned len, quint16 *data, int bytes, QList<TapeEvent> &e); // block
	~TapeChunk();
	TapeChunk(const TapeChunk &other);
	TapeChunk &operator=(const TapeChunk &other);
};

// --------------------------------------------------------------------------
class BlockStore
{
public:
	QMap<unsigned, TapeChunk> store;

	BlockStore(QObject *parent);
	int addChunk(unsigned offset, TapeChunk &c);
	void clear();
	void dump();

};

#endif // BLOCKSTORE_H
