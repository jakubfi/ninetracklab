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

#ifndef __VTAPE_H__
#define __VTAPE_H__

#include <inttypes.h>

enum vt_error_codes {
	VT_OK = 0,
	VT_EOT = -1,
	VT_EOB = -2,
};

#define DEFAULT_BUF_SIZE (1 * 1024 * 1024)

struct vtape {
	char *filename;
	unsigned sample_count;
	uint16_t *data;
	unsigned pos;

	uint16_t *buf;
	int buf_count;

	int nbuckets;
	int *hist;

	int mfp;
	int bpl;
	int bpl_margin;
	int bpl_min;
	int bpl_max;
	int bpl2_min;
	int bpl2_max;
	int skew_max;
};

struct vtape * vtape_open(char *filename, int chmap[9], int downsample);
struct vtape * vtape_make(uint16_t *data, int count);
void vtape_close(struct vtape *t);
void vtape_rewind(struct vtape *t);
void vtape_set_bpl(struct vtape *t, int len, int margin);
void vtape_set_skew(struct vtape *t, int skew_max);
int vtape_get_pulse(struct vtape *t, int *pulse_start, int deskew_max);
void vtape_scan_pulses(struct vtape *t, int nbuckets);

#endif

// vim: tabstop=4 shiftwidth=4 autoindent
