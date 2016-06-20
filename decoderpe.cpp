#include "mainwindow.h"
#include "decoderpe.h"
#include "tapedrive.h"
#include "blockstore.h"

#define DEFAULT_BUF_SIZE 1 * 1024 * 1024

enum burst_search_results {
	B_FAIL,
	B_CONT,
	B_DONE,
};

// --------------------------------------------------------------------------
DecoderPE::DecoderPE()
{
}

// --------------------------------------------------------------------------
int DecoderPE::search_mark(int pulse, int pulse_start)
{
	static int last_result;

	static int mark_count;
	static int last_pulse_start;

	const int bits_recorded = 0b011000100;
	//const int bits_empty = 0b000011010;

	if (last_result != B_CONT) {
		mark_count = 0;
	}

	int time_delta = pulse_start - last_pulse_start;
	last_pulse_start = pulse_start;

	if (
		// it's the mark...
		((pulse & bits_recorded) == bits_recorded)
		// ...and it's on time
		&& ((time_delta >= bpl_min) && (time_delta <= bpl_max))
	) {
		if (mark_count == 0) {
			DBG(4, "mark start\n");
		} else {
			DBG(5, "mark_count++\n");
		}
		mark_count++;
		last_result = B_CONT;
	// it's not the mark
	} else {
		if (mark_count > mark_pulses_min * 2) {
			DBG(4, "mark done\n");
			last_result = B_DONE;
		} else {
			DBG(4, "mark fail\n");
			last_result = B_FAIL;
		}
	}

	return last_result;
}

// --------------------------------------------------------------------------
int DecoderPE::search_preamble(int pulse, int pulse_start)
{
	static int last_result;
	static int zero_count;
	static int last_pulse_start;

	if (last_result != B_CONT) {
		zero_count = 0;
	}

	int time_delta = pulse_start - last_pulse_start;
	last_pulse_start = pulse_start;

	// allow for lost pulses, check most probable cases first (multiple || is faster than popcount)
	int is_sync =
		(pulse == 0b111111111) || (pulse == 0b011111111) || (pulse == 0b111111110) || (pulse == 0b101111111) ||
		(pulse == 0b111111101) || (pulse == 0b110111111) || (pulse == 0b111111011) || (pulse == 0b111011111) ||
		(pulse == 0b111110111) || (pulse == 0b111101111);

	DBG(5, "pulse: %i @ %i len: %i\n", pulse, pulse_start, time_delta);

	// found sync pulse...
	if (is_sync) {
		// ...a short one = 0 => start or continue searching
		if ((time_delta >= bpl_min) && (time_delta <= bpl_max)) {
			DBG(5, "sync_pulses++\n");
			zero_count++;
			last_result = B_CONT;
		// ...a long one = 1 => end of preamble if we have > 25 "0" pulses already
		} else if ((zero_count > sync_pulses_min * 2) && (time_delta >= bpl2_min) && (time_delta <= bpl2_max)) {
			DBG(4, "sync done\n");
			last_result = B_DONE;
		} else {
			DBG(4, "sync fail (pulse length)\n");
			last_result = B_FAIL;
		}
	} else {
			DBG(4, "sync fail (not sync pulse)\n");
		last_result = B_FAIL;
	}

	return last_result;
}

// --------------------------------------------------------------------------
int DecoderPE::find_burst(TapeDrive &td, int *burst_start)
{
	int preamble_start = 0;
	int mark_start = 0;
	int preamble_result = B_FAIL;
	int mark_result = B_FAIL;

	DBG(3, "find (any) burst\n");

	while (1) {
		int pulse_start;
		int pulse = td.read(&pulse_start, deskew, edge_sens);
		if (pulse < VT_OK) {
			return pulse;
		}

		mark_result = search_mark(pulse, pulse_start);
		preamble_result = search_preamble(pulse, pulse_start);

		if (preamble_result == B_CONT) {
			if (!preamble_start) {
				preamble_start = pulse_start;
			}
		} else if (preamble_result == B_DONE) {
			*burst_start = preamble_start;
			return C_BLOCK;
		}

		if (mark_result == B_CONT) {
			if (!mark_start) {
				mark_start = pulse_start;
			}
		} else if (mark_result == B_DONE) {
			*burst_start = mark_start;
			return C_MARK;
		}
	}

	return VT_OK;
}

// --------------------------------------------------------------------------
int DecoderPE::get_row(TapeDrive &td, quint16 *data)
{
	int pulse_start;
	int last_pulse_start = td.get_pos();
	int row_ready = -2;

	while (row_ready < 0) {
		int pulse = td.read(&pulse_start, deskew, edge_sens);
		if (pulse < 0) {
			return pulse;
		}

		int time_delta = pulse_start - last_pulse_start;
		last_pulse_start = pulse_start;

		DBG(5, "data pulse: %i @ %i len %i\n", pulse, pulse_start, time_delta);

		if ((time_delta >= bpl_min) && (time_delta <= bpl_max)) {
			*data ^= pulse;
			row_ready++;
		} else if ((time_delta >= bpl2_min) && (time_delta <= bpl2_max)) {
			*data ^= pulse;
			row_ready += 2;
		} else {
			// this shouldn't happen, everything should be aligned
			// in vtape_get_pulse()
			return VT_EPULSE;
		}
	}

	return VT_OK;
}

// --------------------------------------------------------------------------
int DecoderPE::get_data(TapeDrive &td, BlockStore &bs)
{
	int postamble_rows = 0;
	int row_count = 0;
	quint16 data = 0x01ff; // we start with all ones, as last row of preamble is all ones

	while (1) {
		int res = get_row(td, &data);
		if (res < 0) {
			return res;
		}
		DBG(5, "data: %x (%c)\n", data, data);

		buf[row_count++] = data;

		// start of postamble
		if ((postamble_rows <= 0) && (data == 0b0000000111111111)) {
			DBG(4, "postamble start\n");
			postamble_rows++;
		// inside postamble
		} else if ((postamble_rows >= 1) && (data == 0)) {
			DBG(5, "postamble++\n");
			postamble_rows++;
		// false positive
		} else {
			postamble_rows = 0;
		}
		if (postamble_rows >= sync_pulses_min+1) {
			// cut postamble
			row_count -= postamble_rows;
			DBG(4, "postamble done\n");
			break;
		}
	}

	return row_count;
}

// --------------------------------------------------------------------------
int DecoderPE::get_block(TapeDrive &td, BlockStore &bs)
{
	int burst_start = 0;
	int res;

	DBG(1, "pe_get_block()\n");

	res = find_burst(td, &burst_start);
	if (res == VT_EOT) {
		//vtape_add_eot(t, t->pos);
		return VT_EOT;
	} else if (res == VT_EPULSE) {
		return VT_EPULSE;
	} else if (res == C_BLOCK) {
		DBG(3, "Got preamble, reaing block data\n");
		res = get_data(td, bs);
		if (res >= 0) {
			//vtape_add_block(t, F_PE, burst_start, t->pos - burst_start, buf, res, 0, 0);
			DBG(2, "Block data: '");
			for (int i=0 ; i<res ; i++) {
				//DBG(2, "%c", t->chunk_last->data[i] & 0xff);
			}
			DBG(2, "'\n");
		}
	} else if (res == C_MARK) {
		DBG(3, "Got tape mark\n");
		//vtape_add_mark(t, F_PE, burst_start, t->pos - burst_start);
	}

	return VT_OK;
}

// --------------------------------------------------------------------------
int DecoderPE::run(TapeDrive &td, BlockStore &bs)
{
	while (get_block(td, bs) == VT_OK) {
	}

	return VT_OK;
}

