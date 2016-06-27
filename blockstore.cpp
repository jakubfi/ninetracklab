#include <QDebug>

#include "blockstore.h"
#include "tapedrive.h"

// --------------------------------------------------------------------------
TapeChunk::TapeChunk(long b, long e)
{
	beg = b;
	end = e;
	len = e-b+1;
	crc_tape = 0;
	crc_data = 0;
	hpar_tape = 0;
	hpar_data = 0;
	type = C_NONE;
	format = F_NONE;
	bytes = 0;
	data = NULL;
	vparity = NULL;
	vpar_err_count = 0;
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
	beg = other.beg;
	end = other.end;
	len = other.len;
	crc_tape = other.crc_tape;
	hpar_tape = other.hpar_tape;
	crc_data = other.crc_data;
	hpar_data = other.hpar_data;
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
	crc_tape = other.crc_tape;
	hpar_tape = other.hpar_tape;
	crc_data = other.crc_data;
	hpar_data = other.hpar_data;
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
