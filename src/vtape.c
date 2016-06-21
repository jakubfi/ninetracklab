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
#include "utils.h"

const char *chunk_format_names[] = {
	"PE",
	"NRZ1",
	"NONE",
};

const char *chunk_type_names[] = {
	"NONE",
	"JUNK",
	"BLOCK",
	"TAPE MARK",
	"END OF TAPE",
};

// --------------------------------------------------------------------------
const char * vtape_get_format_name(int format)
{
	return chunk_format_names[format];
}

// --------------------------------------------------------------------------
const char * vtape_get_type_name(int type)
{
	return chunk_type_names[type];
}

// --------------------------------------------------------------------------
void tchunk_drop(struct tchunk *chunk)
{
	if (!chunk) return;

	free(chunk->data);
	free(chunk);
}

// --------------------------------------------------------------------------
static void chunk_add(struct vtape *t, struct tchunk *chunk)
{
	if (t->chunk_last) {
		t->chunk_last->next = chunk;
	} else {
		t->chunk_first = chunk;
	}

	t->chunk_last = chunk;
}

// --------------------------------------------------------------------------
int vtape_add_block(struct vtape *t, int format, int offset, int samples, uint16_t *buf, int count, int crc, int hparity)
{
	struct tchunk *chunk = calloc(1, sizeof(struct tchunk));
	chunk->type = C_BLOCK;
	chunk->format = format;
	chunk->offset = offset;
	chunk->samples = samples;
	chunk->data = malloc(count * sizeof(uint16_t));
	memcpy(chunk->data, buf, count * sizeof(uint16_t));
	chunk->len = count;
	t->blocks[format]++;
	chunk->hparity = hparity;
	chunk->crc = crc;
	chunk_add(t, chunk);

	return 0;
}

// --------------------------------------------------------------------------
int vtape_add_mark(struct vtape *t, int format, int offset, int samples)
{
	struct tchunk *chunk = calloc(1, sizeof(struct tchunk));
	chunk->type = C_MARK;
	chunk->offset = offset;
	chunk->samples = samples;
	t->marks[format]++;
	chunk_add(t, chunk);

	return 0;
}

// --------------------------------------------------------------------------
int vtape_add_eot(struct vtape *t, int offset)
{
	struct tchunk *chunk = calloc(1, sizeof(struct tchunk));
	chunk->type = C_EOT;
	chunk->offset = offset;
	chunk_add(t, chunk);

	return 0;
}

// --------------------------------------------------------------------------
struct vtape * vtape_open(char *filename, int chmap[9], int downsample, int sdeskew[9])
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

	// TODO: this needs to be re-optimized
	// remap channels so tracks are in order: p, 7, 6, 5, 4, 3, 2, 1, 0
	// downsample data
	// apply static deskew
	for (int i=0 ; i<t->sample_count ; i++) {
		int d_o = 0;
		for (int c=0 ; c<9 ; c++) {
			int deskewed_position = i - sdeskew[chmap[8-c]];
			int source_data = source[deskewed_position * downsample];
			int source_bit = (source_data >> chmap[8-c]) & 1;
			d_o |= source_bit << c;
		}
		t->data[i] = d_o;
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

	struct tchunk *ch = t->chunk_first;
	struct tchunk *chn;
	while (ch) {
		chn = ch->next;
		tchunk_drop(ch);
		ch = chn;
	}

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
	t->bpl4_min = 4*len - margin;
	t->bpl4_max = 4*len + margin;
	t->bpl8_min = 8*len - margin;
	t->bpl8_max = 8*len + margin;
}

// --------------------------------------------------------------------------
void vtape_set_deskew(struct vtape *t, int ddynamic)
{
	t->deskew_dynamic = ddynamic;
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
		//pulse = t->data[t->pos] ^ t->data[t->pos+1];
		pulse = (t->data[t->pos] ^ t->data[t->pos+1]) & t->data[t->pos+1];
		t->pos++;
	} while (!pulse);

	*pulse_start = t->pos;
	int actual_deskew = t->pos;

	// as long as we are in the deskew window
	while (deskew_max > 0) {
		if (t->pos >= t->sample_count-1) {
			break;
		}

		// check next sample for edges
		//next_pulse = t->data[t->pos] ^ t->data[t->pos+1];
		next_pulse = (t->data[t->pos] ^ t->data[t->pos+1]) & t->data[t->pos+1];

		// if signal bounces back in the deskew window, treat it as a separete edge
		deskew_bounced = pulse & next_pulse;
		if (deskew_bounced) {
			VTDEBUG(9, "deskew bounce\n");
			break;
		}

		// glue edges in the deskew window
		pulse |= next_pulse;

		if (next_pulse) {
			actual_deskew = t->pos;
		}

		deskew_max--;
		t->pos++;
	}

	VTDEBUG(9, "deskewed pulse %i @ %i deskew len %i\n", pulse, *pulse_start, actual_deskew - *pulse_start);

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

// vim: tabstop=4 shiftwidth=4 autoindent
