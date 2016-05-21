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

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>

#include "vtape.h"

// --------------------------------------------------------------------------
int term_width()
{
	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	return w.ws_col;
}

// --------------------------------------------------------------------------
void print_stats(struct vtape *t)
{
	unsigned width = term_width() - 8;

	if (!t->mfp) {
		return;
	}

	char *line = malloc(width+1);
	memset(line, '#', width);
	line[width] = '\0';

	for (int i=0 ; i<t->nbuckets; i++) {
		if (t->hist[i] > 0) {
			double len = (double) t->hist[i] / (double) t->hist[t->mfp] * (double) width + 0.5;
			printf("%4i : %s\n", i, line + (width-(int)len));
		}
	}

	free(line);
}

// --------------------------------------------------------------------------
int main(int argc, char **argv)
{
	setbuf(stdout, NULL);

	uint16_t test_tape[] = {
	0b0000000000000000, // 0
	0b0000000000000000, // 1
	0b0000000000000111, // 2
	0b0000000000000111, // 3
	0b0000000000000111, // 4
	0b0000000000000101, // 5
	0b0000000000000010, // 6
	0b0000000000000000, // 7
	0b0000000000000000, // 8
	0b0000000000000000, // 9
	0b0000000000000111, // 10
	0b0000000000000111, // 11
	0b0000000000000111, // 12
	0b0000000000000111, // 13
	0b0000000000000000, // 14
	0b0000000000000000, // 15
	};

//	char *filename = "data/zcd5-env5.bin";
//	char *filename = "data/fake.bin";
//	char *filename = "data/fake_dense_100M.bin";
	char *filename = "data/fake_dense_1G.bin";

	char *ch_names[] = { "P", "8", "7", "6", "5", "4", "3", "2", "1", "0" };
	int downsample = 1;
	int chmap[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	printf("Loading tape image '%s' with channel order (parity, msb, ..., lsb): ", filename);
	for (int i=0 ; i<9 ; i++) {
		printf("%i", chmap[i]);
		if (i<8) {
			printf(", ");
		}
	}
	printf("\n");

	struct vtape *t = vtape_open(filename, chmap, downsample);
	printf("Loaded %i samples (downsample factor: 1/%i)\n", t->sample_count, downsample);

//	int chmap[9] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
//	struct vtape *t = vtape_make(test_tape, 16, chmap);

	printf("Analyzing pulse lengths... ");
	vtape_scan_pulses(t, 1000);
	printf("most frequent pulse is %i samples\n", t->mfp);
	printf("Pulse length histogram:\n");
	print_stats(t);
	vtape_set_bpl(t, 6, 2);
	vtape_set_skew(t, 1);

	printf("PE analyzer initialized with: bpl=%i, margin=%i, short_pulse=[%i..%i], long_pulse=[%i..%i], skew_max=%i\n", t->bpl, t->bpl_margin, t->bpl_min, t->bpl_max, t->bpl2_min, t->bpl2_max, t->skew_max);


	vtape_rewind(t);

	printf("Running PE analysis... ");
	int blocks = vtape_analyze_pe(t);
	printf("got %i PE blocks\n", blocks);

	vtape_close(t);

	return 0;
}

// vim: tabstop=4 shiftwidth=4 autoindent
