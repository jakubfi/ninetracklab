#include <QDebug>

#include "blockstore.h"
#include "tapedrive.h"

// --------------------------------------------------------------------------
TapeChunk::TapeChunk(long b, long e)
{
	beg = b;
	end = e;
	len = e-b+1;
	b_crc = 0;
	d_crc = 0;
	b_hparity = 0;
	d_hparity = 0;
	type = C_NONE;
	format = F_NONE;
	bytes = 0;
	data = NULL;
	vparity = NULL;
	vparity_errors = 0;
}

// --------------------------------------------------------------------------
TapeChunk::~TapeChunk()
{
	delete[] data;
	delete[] vparity;
}

// --------------------------------------------------------------------------
TapeChunk::TapeChunk()
{
	TapeChunk(0, 0);
}

// --------------------------------------------------------------------------
TapeChunk::TapeChunk(const TapeChunk &other)
{
	beg = other.beg;
	end = other.end;
	len = other.len;
	b_crc = other.b_crc;
	b_hparity = other.b_hparity;
	d_crc = other.d_crc;
	d_hparity = other.d_hparity;
	type = other.type;
	format = other.format;
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
	beg = other.beg;
	end = other.end;
	len = other.len;
	b_crc = other.b_crc;
	b_hparity = other.b_hparity;
	d_crc = other.d_crc;
	d_hparity = other.d_hparity;
	type = other.type;
	format = other.format;
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
