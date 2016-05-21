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
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/ioctl.h>

#include "vtape.h"

// --------------------------------------------------------------------------
struct vtape * vtape_open(char *filename, int chmap[9], int downsample)
{
	struct vtape *t = calloc(1, sizeof(struct vtape));

	struct stat st;
	stat(filename, &st);
	int input_samples = st.st_size / 2;
	t->sample_count = input_samples / downsample;
	t->filename = strdup(filename);

	int fd = open(filename, O_RDONLY);
	uint16_t *source = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	t->data = calloc(t->sample_count, sizeof(uint16_t));

	// remap channels so tracks are in order: p, 7, 6, 5, 4, 3, 2, 1, 0
	// (two separate loops because performance...)
	if (downsample > 1) {
		for (int i=0 ; i<t->sample_count ; i++) {
			int d_o = 0;
			for (int c=0 ; c<9 ; c++) {
				d_o |= ((source[i*downsample] >> chmap[c]) & 1) << c;
			}
			t->data[i] = d_o;
		}
	} else {
		for (int i=0 ; i<t->sample_count ; i++) {
			int d_o = 0;
			for (int c=0 ; c<9 ; c++) {
				d_o |= ((source[i] >> chmap[c]) & 1) << c;
			}
			t->data[i] = d_o;
		}
	}

	munmap(source, st.st_size);
	close(fd);
	return t;
}

// --------------------------------------------------------------------------
struct vtape * vtape_make(uint16_t *data, int count)
{
	struct vtape *t = calloc(1, sizeof(struct vtape));

	t->sample_count = count;
	t->filename = strdup("TEST TAPE");
	t->data = malloc(count * sizeof(uint16_t));
	memcpy(t->data, data, count * sizeof(uint16_t));

	return t;
}

// --------------------------------------------------------------------------
void vtape_close(struct vtape *t)
{
	if (!t) return;

	free(t->data);
	free(t->filename);
	free(t->hist);
	free(t);
}

// --------------------------------------------------------------------------
void vtape_rewind(struct vtape *t)
{
	t->pos = 0;
}

// --------------------------------------------------------------------------
void vtape_set_bpl(struct vtape *t, int len, int margin)
{
	t->bpl = len;
	t->bpl_margin = margin;
	t->bpl_min = len - margin;
	t->bpl_max = len + margin;
	t->bpl2_min = 2*len - margin;
	t->bpl2_max = 2*len + margin;
}

// --------------------------------------------------------------------------
void vtape_set_skew(struct vtape *t, int skew_max)
{
	t->skew_max = skew_max;
}

// --------------------------------------------------------------------------
int vtape_get_pulse(struct vtape *t, int *pulse_start, int deskew_max)
{
	int pulse;
	int next_pulse;
	int deskew_bounced;

	// find edge
	do {
		if (t->pos >= t->sample_count-1) {
			return VT_EOT;
		}
		pulse = t->data[t->pos] ^ t->data[t->pos+1];
		t->pos++;
	} while (!pulse);

	*pulse_start = t->pos;

	// as long as we are in the deskew window
	while (deskew_max > 0) {
		if (t->pos >= t->sample_count-1) {
			break;
		}

		// check next sample for edges
		next_pulse = t->data[t->pos] ^ t->data[t->pos+1];

		// if signal bounces back in the deskew window, treat it as a separete edge
		deskew_bounced = pulse & next_pulse;
		if (deskew_bounced) {
			break;
		}

		// glue edges in the deskew window
		pulse |= next_pulse;

		deskew_max--;
		t->pos++;
	}

	return pulse;
}

// --------------------------------------------------------------------------
void vtape_scan_pulses(struct vtape *t, int nbuckets)
{
	int pulse;
	int last_change[9] = { 0,0,0,0,0,0,0,0,0 };
	int pulse_start;

	t->nbuckets = nbuckets;
	t->hist = calloc(1, nbuckets * sizeof(int));

	while ((pulse = vtape_get_pulse(t, &pulse_start, 0)) != VT_EOT) {

		for (int c=0 ; c<9 ; c++) {
			if (pulse & (1<<c)) {
				int len = t->pos - last_change[c];
				if (len < nbuckets) {
					t->hist[len]++;
				}
				last_change[c] = t->pos;
			}
		}
	}

	int most_frequent = 0;
	for (int i=0 ; i<t->nbuckets; i++) {
		if (t->hist[i] > most_frequent) {
			t->mfp = i;
			most_frequent = t->hist[i];
		}
	}
}

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
int vtape_analyze_pe(struct vtape *t)
{
	int blocks = 0;
	while (vtape_get_pe_preamble(t) == VT_OK) {
		blocks++;
	}

	return blocks;
}


// vim: tabstop=4 shiftwidth=4 autoindent
