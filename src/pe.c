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

#include <stdlib.h>

#include "vtape.h"

// --------------------------------------------------------------------------
int vtape_get_pe_preamble(struct vtape *t)
{
	int pulse_start;
	int pulse = 0;
	int zero_count = 0;
	int last_pulse_start = 0;
	int preamble_start = 0;

	while (1) {
		pulse = vtape_get_pulse(t, &pulse_start, t->skew_max);
		if (pulse < VT_OK) {
			return pulse;
		}

		int time_delta = pulse_start - last_pulse_start;

		// allow for lost pulses, check most probable cases first (multiple || is faster than popcount)
		int is_sync =
			(pulse == 0b111111111) || (pulse == 0b011111111) || (pulse == 0b111111110) || (pulse == 0b101111111) ||
			(pulse == 0b111111101) || (pulse == 0b110111111) || (pulse == 0b111111011) || (pulse == 0b111011111) ||
		  	(pulse == 0b111110111) || (pulse == 0b111101111);

		// found sync pulse...
		if (is_sync) {
			// ...a short one = 0 => continue searching
			if ((time_delta >= t->bpl_min) && (time_delta <= t->bpl_max)) {
				if (!preamble_start) {
					preamble_start = pulse_start;
				}
				zero_count++;
			// ...a long one = 1 => end of preamble if we have > 25 "0" pulses already
			} else if ((zero_count > 50) && (time_delta >= t->bpl2_min) && (time_delta <= t->bpl2_max)) {
				break;
			// wrong length => start over
			} else {
				zero_count = 0;
			}
		// not a sync pulse => start over
		} else {
			zero_count = 0;
		}

		last_pulse_start = pulse_start;
	}

	return VT_OK;
}

// --------------------------------------------------------------------------
int vtape_get_pe_block(struct vtape *t)
{
	return vtape_get_pe_preamble(t);
}

// --------------------------------------------------------------------------
int vtape_analyze_pe(struct vtape *t)
{
	int blocks = 0;
	while (vtape_get_pe_block(t) == VT_OK) {
		blocks++;
	}

	return blocks;
}


// vim: tabstop=4 shiftwidth=4 autoindent
