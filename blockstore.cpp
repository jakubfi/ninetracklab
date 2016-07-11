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
	fixed = 0;
	type = C_NONE;
	format = F_NONE;
	bytes = 0;
	data = NULL;
	vpar_err_count = 0;
	hpar_err_count = 0;
}

// --------------------------------------------------------------------------
TapeChunk::~TapeChunk()
{
	delete[] data;
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
	fixed = other.fixed;
	type = other.type;
	format = other.format;
	bytes = other.bytes;
	if (other.data) {
		data = new quint16[bytes];
		for (int i=0 ; i<bytes ; i++) {
			data[i] = other.data[i];
		}
	} else {
		data = NULL;
	}
	events = other.events;
	hpar_err_count = other.hpar_err_count;
	vpar_err_count = other.vpar_err_count;
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
	fixed = other.fixed;
	type = other.type;
	format = other.format;
	bytes = other.bytes;
	delete[] data;
	if (other.data) {
		data = new quint16[bytes];
		for (int i=0 ; i<bytes ; i++) {
			data[i] = other.data[i];
		}
	} else {
		data = NULL;
	}
	events = other.events;
	hpar_err_count = other.hpar_err_count;
	vpar_err_count = other.vpar_err_count;
	return *this;
}

// vim: tabstop=4 shiftwidth=4 autoindent
