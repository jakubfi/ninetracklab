#include <QtDebug>
#include <QTime>

#include "ninetracklab.h"
#include "tapedrive.h"
#include "decodernrz1.h"

// --------------------------------------------------------------------------
DecoderNRZ1::DecoderNRZ1(TapeDrive *td)
{
	this->td = td;
}

// --------------------------------------------------------------------------
quint16 DecoderNRZ1::crc(quint16 *data, int size)
{
	int i = 0;
	quint16 reg = 0;
	int bit;

	while (i < size) {
		reg ^= data[i++];
//again:
		bit = reg & 1;
		reg >>= 1;
		reg |= bit << 8;
		if (bit == 1) {
			reg ^= 0b000111100;
		}

		if ((i == size) && (reg & 0b100000000)) {
			//goto again;
		}
	}

	return reg ^ 0b111010111;
}

// --------------------------------------------------------------------------
TapeChunk DecoderNRZ1::scan_next_chunk(int start)
{
	int pulse_start;
	int chunk_start = -1;
	int chunk_end;

	td->seek(start, TD_SEEK_SET);

	forever {
		int pulse = td->read(&pulse_start, td->cfg.deskew, td->cfg.edge_sens);
		int distance = pulse_start - start;
		if ((distance > td->cfg.bpl*8) || (start == 0) || (pulse < 0)) {
			if (chunk_start < 0) {
				if (pulse < 0) {
					return TapeChunk(-1, -1);
				}
				chunk_start = pulse_start - 4*25;
			} else {
				chunk_end = start + 4*25;
				TapeChunk chunk = TapeChunk(chunk_start, chunk_end);
				return chunk;
			}
		}
		start = pulse_start;
	}
	return TapeChunk(-1, -1);
}

// --------------------------------------------------------------------------
int DecoderNRZ1::process(TapeChunk &chunk)
{
	int pulse_start = chunk.beg;
	int last_pulse_start = 0;
	int rowcount = 0;
	int b_crc = -1;
	int d_hparity = 0;

	const quint16 tape_mark = 0b000010011;

	td->seek(chunk.beg, TD_SEEK_SET);
	chunk.events.clear();
	chunk.type = C_NONE;
	chunk.format = F_NONE;
	chunk.vparity_errors = 0;

	while (pulse_start < chunk.end) {
		int pulse = td->read(&pulse_start, td->cfg.deskew, td->cfg.edge_sens);
		if (pulse < 0) break;

		int time_delta = pulse_start - last_pulse_start;

		// hparity and crc pulse
		if ((rowcount > 1) && (time_delta >= td->cfg.bpl*2.5) && (time_delta <= td->cfg.bpl*6)) {
			// hparity
			if (b_crc != -1) {
				chunk.events.append(TapeEvent(pulse_start, C_ROW));
				chunk.b_hparity = pulse;
				chunk.d_hparity = d_hparity;
				chunk.b_crc = b_crc;
				chunk.d_crc = crc(buf, rowcount);
				chunk.type = C_BLOCK;
				chunk.format = F_NRZ1;
				chunk.bytes = rowcount;
				chunk.data = new quint8[rowcount];
				qCopy(buf, buf+rowcount, chunk.data);
				return VT_OK;
			// crc
			} else {
				b_crc = pulse;
				d_hparity ^= pulse;
				chunk.events.append(TapeEvent(pulse_start, C_ROW));
			}
		// tape mark
		} else if ((pulse == tape_mark) && (time_delta >= td->cfg.bpl*6.1) && (time_delta <= td->cfg.bpl*10) && (rowcount == 1) && (buf[rowcount-1] == tape_mark)) {
			chunk.type = C_MARK;
			chunk.format = F_NRZ1;
			chunk.events.append(TapeEvent(pulse_start, C_ROW));
			return VT_OK;
		// data
		} else {
			// discard too long pulses
			if ((rowcount == 1) && (time_delta >= td->cfg.bpl*2.5)) {
				rowcount = 0;
				d_hparity = 0;
				chunk.events.clear();
			}
			d_hparity ^= pulse;
			int vp = !(td->parity9(pulse) ^ (pulse>>8));
			if (vp) {
				chunk.events.append(TapeEvent(pulse_start, C_ERROR));
				chunk.vparity_errors++;
			} else {
				chunk.events.append(TapeEvent(pulse_start, C_ROW));
			}
			buf[rowcount] = pulse;
			rowcount++;
		}

		last_pulse_start = pulse_start;
	}

	return VT_EOT;
}


