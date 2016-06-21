#include <QDebug>

#include "blockstore.h"
#include "tapedrive.h"

// --------------------------------------------------------------------------
TapeChunk::TapeChunk(unsigned len, QList<TapeEvent> &e)
{
	this->b_crc = 0;
	this->d_crc = 0;
	this->b_hparity = 0;
	this->d_hparity = 0;
	this->type = C_GAP;
	this->format = F_NONE;
	this->len = len;
	this->bytes = 0;
	this->data = NULL;
	this->vparity = NULL;
	this->events = e;
}

// --------------------------------------------------------------------------
TapeChunk::TapeChunk(int format, unsigned len, QList<TapeEvent> &e)
{
	this->b_crc = 0;
	this->d_crc = 0;
	this->b_hparity = 0;
	this->d_hparity = 0;
	this->type = C_MARK;
	this->format = format;
	this->len = len;
	this->bytes = 0;
	this->data = NULL;
	this->vparity = NULL;
	this->events = e;
}

// --------------------------------------------------------------------------
TapeChunk::TapeChunk(int format, unsigned len, quint16 *data, int bytes, QList<TapeEvent> &e)
{
	this->b_crc = 0;
	this->d_crc = 0;
	this->b_hparity = 0;
	this->d_hparity = 0;
	this->type = C_BLOCK;
	this->format = format;
	this->len = len;
	this->bytes = bytes;
	this->data = new quint8[bytes];
	this->vparity = new quint8[bytes];
	for (int i=0 ; i<bytes ; i++) {
		this->data[i] = data[i];
		this->vparity[i] = data[i] >> 8;
	}
	this->events = e;
}

// --------------------------------------------------------------------------
TapeChunk::~TapeChunk()
{
	delete[] data;
	delete[] vparity;
}

// --------------------------------------------------------------------------
TapeChunk::TapeChunk(const TapeChunk &other)
{
	b_crc = other.b_crc;
	b_hparity = other.b_hparity;
	d_crc = other.d_crc;
	d_hparity = other.d_hparity;
	type = other.type;
	format = other.format;
	len = other.len;
	bytes = other.bytes;
	if (other.data) {
		data = new quint8[bytes];
		vparity = new quint8[bytes];
		for (int i=0 ; i<bytes ; i++) {
			data[i] = other.data[i];
			vparity[i] = other.data[i] >> 8;
		}
	} else {
		data = NULL;
		vparity = NULL;
	}
	events = other.events;
}

// --------------------------------------------------------------------------
TapeChunk & TapeChunk::operator=(const TapeChunk &other)
{
	b_crc = other.b_crc;
	b_hparity = other.b_hparity;
	d_crc = other.d_crc;
	d_hparity = other.d_hparity;
	type = other.type;
	format = other.format;
	len = other.len;
	bytes = other.bytes;
	delete[] data;
	if (other.data) {
		data = new quint8[bytes];
		vparity = new quint8[bytes];
		for (int i=0 ; i<bytes ; i++) {
			data[i] = other.data[i];
			vparity[i] = other.data[i] >> 8;
		}
	} else {
		data = NULL;
		vparity = NULL;
	}
	events = other.events;
	return *this;
}


// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------


// --------------------------------------------------------------------------
BlockStore::BlockStore(QObject *parent)
{

}

// --------------------------------------------------------------------------
void BlockStore::clear()
{
	store.clear();
}

// --------------------------------------------------------------------------
int BlockStore::addChunk(unsigned offset, TapeChunk &c)
{
	QMap<unsigned, TapeChunk>::iterator i;

	// remove overlapping elements
	while (!store.isEmpty()) {
		i = store.lowerBound(offset+c.len) - 1;
		if (offset < i.key()+i.value().len) {
			store.remove(i.key());
		} else {
			break;
		}
	}

	store.insert(offset, c);

	return 0;
}

// --------------------------------------------------------------------------
void BlockStore::dump()
{
	QMap<unsigned, TapeChunk>::iterator i;
	for (i=store.begin() ; i!=store.end() ; i++) {
		qDebug() << i.key() << ": " << i.value().bytes << endl;
	}

}
