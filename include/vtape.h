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
	VT_EPULSE = -2,
};

enum chunk_formats {
	F_PE,
	F_NRZ1,
	F_NONE,
};

enum chunk_types {
	C_NONE,
	C_JUNK,
	C_BLOCK,
	C_MARK,
	C_EOT,
};

struct tchunk {
	int offset;
	int samples;
	int type;
	int format;
	uint16_t *data;
	int len;
	struct tchunk *next;
};

struct vtape {
	char *filename;
	unsigned sample_count;
	uint16_t *data;
	unsigned pos;

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

	struct tchunk *chunk_first, *chunk_last;
	int chunks;
	int blocks[2];
	int marks[2];
};

void tchunk_drop(struct tchunk *chunk);
int vtape_add_block(struct vtape *t, int format, int offset, int samples, uint16_t *buf, int count);
int vtape_add_mark(struct vtape *t, int format, int offset, int samples);
int vtape_add_eot(struct vtape *t, int offset);
const char * vtape_get_format_name(int format);
const char * vtape_get_type_name(int type);

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
