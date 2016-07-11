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
	int got_crc = 0;

	const quint16 tape_mark = 0b000010011;
	const quint16 tape_mark_old = 0b111010111;

	td->seek(chunk.beg, TD_SEEK_SET);

	chunk.events.clear();
	chunk.type = C_NONE;
	chunk.format = F_NONE;
	chunk.vpar_err_count = 0;
	chunk.hpar_err_count = 0;
	chunk.bytes = 0;
	chunk.hpar_data = 0;
	chunk.hpar_tape = 0;
	chunk.crc_data = 0;
	chunk.crc_tape = 0;
	chunk.fixed = 0;

	while (pulse_start < chunk.end) {
		int pulse = td->read(&pulse_start, td->cfg.deskew, td->cfg.edge_sens);
		if (pulse < 0) break;

		int time_delta = pulse_start - last_pulse_start;

		// hparity and crc pulse
		if ((chunk.bytes > 1) && (time_delta >= td->cfg.bpl*2.5) && (time_delta <= td->cfg.bpl*6)) {
			// hparity
			if (got_crc) {
				chunk.events.append(TapeEvent(pulse_start, C_ROW));
				chunk.hpar_tape = pulse;
				chunk.hpar_err = chunk.hpar_data ^ chunk.hpar_tape;
				chunk.crc_data = crc(buf, chunk.bytes);
				chunk.type = C_BLOCK;
				chunk.format = F_NRZ1;
				chunk.data = new quint16[chunk.bytes];
				qCopy(buf, buf+chunk.bytes, chunk.data);
				for (int i=0 ; i<9 ; i++) {
					if ((chunk.hpar_err>>i)&1) chunk.hpar_err_count++;
				}
				return VT_OK;
			// crc
			} else {
				chunk.crc_tape = pulse;
				chunk.hpar_data ^= pulse;
				chunk.events.append(TapeEvent(pulse_start, C_ROW));
				got_crc = 1;
			}
		// old tapemark
		} else if ((pulse == tape_mark_old) && (time_delta >= td->cfg.bpl*2.5) && (time_delta <= td->cfg.bpl*6) && (chunk.bytes == 1) && (buf[chunk.bytes-1] == tape_mark_old)) {
			chunk.type = C_MARK;
			chunk.format = F_NRZ1;
			chunk.hpar_data = 0;
			chunk.bytes = 0;
			chunk.events.append(TapeEvent(pulse_start, C_ROW));
			return VT_OK;
		// standard tape mark
		} else if ((pulse == tape_mark) && (time_delta >= td->cfg.bpl*6.1) && (time_delta <= td->cfg.bpl*10) && (chunk.bytes == 1) && (buf[chunk.bytes-1] == tape_mark)) {
			chunk.type = C_MARK;
			chunk.format = F_NRZ1;
			chunk.hpar_data = 0;
			chunk.bytes = 0;
			chunk.events.append(TapeEvent(pulse_start, C_ROW));
			return VT_OK;
		// data
		} else {
			// discard too long pulses
			if ((chunk.bytes == 1) && (time_delta >= td->cfg.bpl*2.5)) {
				chunk.bytes = 0;
				chunk.hpar_data = 0;
				chunk.events.clear();
			}
			chunk.hpar_data ^= pulse;
			int vp = !(td->parity9(pulse) ^ (pulse>>8));
			if (vp) {
				chunk.events.append(TapeEvent(pulse_start, C_ERROR));
				chunk.vpar_err_count++;
			} else {
				chunk.events.append(TapeEvent(pulse_start, C_ROW));
			}
			buf[chunk.bytes] = pulse;
			chunk.bytes++;
		}

		last_pulse_start = pulse_start;
	}

	return VT_EOT;
}

// vim: tabstop=4 shiftwidth=4 autoindent
