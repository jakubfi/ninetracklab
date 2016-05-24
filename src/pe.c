//  Copyright (c) 2016 Jakub Filipowicz <jakubf@gmail.com>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc.,
//  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

#define _XOPEN_SOURCE 500

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "vtape.h"
#include "utils.h"

#define DEFAULT_BUF_SIZE 1 * 1024 * 1024

enum burst_search_results {
	B_FAIL,
	B_CONT,
	B_DONE,
};

// --------------------------------------------------------------------------
static int pe_search_mark(struct vtape *t, int pulse, int pulse_start)
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
		&& ((time_delta >= t->bpl_min) && (time_delta <= t->bpl_max))
	) {
		if (mark_count == 0) {
			VTDEBUG("mark start\n");
		} else {
			VTDEBUG("mark cont\n");
		}
		mark_count++;
		last_result = B_CONT;
	// it's not the mark
	} else {
		if (mark_count > 64 * 2) {
			VTDEBUG("mark done\n");
			last_result = B_DONE;
		} else {
			VTDEBUG("mark fail\n");
			last_result = B_FAIL;
		}
	}

	return last_result;
}

// --------------------------------------------------------------------------
static int pe_search_preamble(struct vtape *t, int pulse, int pulse_start)
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

	// found sync pulse...
	if (is_sync) {
		// ...a short one = 0 => start or continue searching
		if ((time_delta >= t->bpl_min) && (time_delta <= t->bpl_max)) {
			zero_count++;
			last_result = B_CONT;
		// ...a long one = 1 => end of preamble if we have > 25 "0" pulses already
		} else if ((zero_count > 25 * 2) && (time_delta >= t->bpl2_min) && (time_delta <= t->bpl2_max)) {
			last_result = B_DONE;
		} else {
			last_result = B_FAIL;
		}
	} else {
		last_result = B_FAIL;
	}

	return last_result;
}

// --------------------------------------------------------------------------
static int pe_find_burst(struct vtape *t, int *burst_start)
{
	int preamble_start = 0;
	int mark_start = 0;
	int preamble_result = B_FAIL;
	int mark_result = B_FAIL;

	VTDEBUG("find burst\n");

	while (1) {
		int pulse_start;
		int pulse = vtape_get_pulse(t, &pulse_start, t->skew_max);
		if (pulse < VT_OK) {
			return pulse;
		}

		mark_result = pe_search_mark(t, pulse, pulse_start);
		preamble_result = pe_search_preamble(t, pulse, pulse_start);

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
static int pe_get_row(struct vtape *t, uint16_t *data)
{
	int pulse_start;
	int last_pulse_start = t->pos;
	int row_ready = -2;

	while (row_ready < 0) {
		int pulse = vtape_get_pulse(t, &pulse_start, t->skew_max);
		if (pulse < 0) {
			return pulse;
		}

		int time_delta = pulse_start - last_pulse_start;
		last_pulse_start = pulse_start;

		if ((time_delta >= t->bpl_min) && (time_delta <= t->bpl_max)) {
			*data ^= pulse;
			row_ready++;
		} else if ((time_delta >= t->bpl2_min) && (time_delta <= t->bpl2_max)) {
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
static int pe_get_data(struct vtape *t, uint16_t *buf)
{
	int postamble_rows = 0;
	int row_count = 0;
	uint16_t data = 0x01ff; // we start with all ones, as last row of preamble is all ones

	VTDEBUG("reading data\n");

	while (1) {
		int res = pe_get_row(t, &data);
		if (res < 0) {
			return res;
		}
		VTDEBUG("data: %x (%c)\n", data, data);

		buf[row_count++] = data;

		// start of postamble
		if ((postamble_rows <= 0) && (data == 0b0000000111111111)) {
			VTDEBUG("postamble start\n");
			postamble_rows++;
		// inside postamble
		} else if ((postamble_rows >= 1) && (data == 0)) {
			postamble_rows++;
		// false positive
		} else {
			postamble_rows = 0;
		}
		if (postamble_rows >= 41) {
			// cut postamble
			row_count -= postamble_rows;
			VTDEBUG("postamble done\n");
			break;
		}
	}

	return row_count;
}

// --------------------------------------------------------------------------
int pe_get_block(struct vtape *t, uint16_t *buf)
{
	int burst_start = 0;
	int res;

	VTDEBUG("find burst\n");

	res = pe_find_burst(t, &burst_start);
	if (res == VT_EOT) {
		vtape_add_eot(t, t->pos);
		return VT_EOT;
	} else if (res == VT_EPULSE) {
		return VT_EPULSE;
	} else if (res == C_BLOCK) {
		VTDEBUG("found preamble\n");
		res = pe_get_data(t, buf);
		if (res >= 0) {
			vtape_add_block(t, F_PE, burst_start, t->pos - burst_start, buf, res);
			VTDEBUG("Chunk data: '");
			for (int i=0 ; i<res ; i++) {
				VTDEBUG("%c", t->chunk_last->data[i] & 0xff);
			}
			VTDEBUG("'\n");
		}
	} else if (res == C_MARK) {
		VTDEBUG("found mark\n");
		vtape_add_mark(t, F_PE, burst_start, t->pos - burst_start);
	}

	return VT_OK;
}

// --------------------------------------------------------------------------
int pe_analyze(struct vtape *t)
{
	// data buffer
	uint16_t *buf = malloc(DEFAULT_BUF_SIZE * sizeof(uint16_t));

	while (pe_get_block(t, buf) == VT_OK) {
	}

	return VT_OK;
}


// vim: tabstop=4 shiftwidth=4 autoindent
