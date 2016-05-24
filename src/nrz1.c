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

// --------------------------------------------------------------------------
int nrz1_get_block(struct vtape *t, uint16_t *buf)
{
	int pulse_start;
	int last_pulse_start = t->pos;
	int got_crc = 0;
	int rowcount = 0;
	int crc;
	int hparity;
	int block_start = t->pos;

	VTDEBUG("get block\n");

	while (1) {

		int pulse = vtape_get_pulse(t, &pulse_start, t->skew_max);
		if (pulse < 0) {
			return pulse;
		}

		int time_delta = pulse_start - last_pulse_start;

		// data pulse
		if ((rowcount == 0) || ((time_delta >= t->bpl_min) && (time_delta <= t->bpl_max))) {
			VTDEBUG("data: 0x%04x (%c), %i\n", pulse, pulse&0xff, time_delta);
			buf[rowcount++] = pulse;
		// hparity and crc pulse
		} else if ((time_delta >= t->bpl4_min) && (time_delta <= t->bpl4_max)) {
			if (got_crc) {
				VTDEBUG("hparity: 0x%04x, %i\n", pulse, time_delta);
				hparity = pulse;
				vtape_add_block(t, F_NRZ1, block_start, t->pos - block_start, buf, rowcount, crc, hparity);
				return VT_OK;
			} else {
				VTDEBUG("crc: 0x%04x, %i\n", pulse, time_delta);
				crc = pulse;
				got_crc = 1;
			}
		// tape mark
		} else if (
			(pulse == 0b10100100)
			&& (time_delta >= t->bpl8_min) && (time_delta <= t->bpl8_max
			&& (rowcount == 1) && (buf[rowcount-1] == 0b10100100))
		) {
			VTDEBUG("tape mark: 0x%04x, %i\n", pulse, time_delta);
			return VT_OK;
		// bad pulse
		} else {
			VTDEBUG("bad pulse: 0x%04x, %i\n", pulse, time_delta);
		}

		last_pulse_start = pulse_start;

	}

	return VT_OK;
}

// --------------------------------------------------------------------------
int nrz1_analyze(struct vtape *t)
{
	// data buffer
	uint16_t *buf = malloc(DEFAULT_BUF_SIZE * sizeof(uint16_t));

	while (nrz1_get_block(t, buf) == VT_OK) {
	}

	free(buf);

	return VT_OK;
}


// vim: tabstop=4 shiftwidth=4 autoindent
