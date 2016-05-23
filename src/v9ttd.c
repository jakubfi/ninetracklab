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
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <getopt.h>

#include "utils.h"
#include "vtape.h"
#include "pe.h"

#define DEFAULT_SKEW 0.1f
#define DEFAULT_MARGIN 0.4f

/*
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
*/

enum actions {
	A_UNKNOWN	= 0,
	A_QUIT		= 1<<0,
	A_STATS		= 1<<1,
	A_ANALYZE	= 1<<2,
	A_HELP		= 1<<3,
};

struct config {
	char *input_name;
	char *output_name;
	int chmap[9];
	int downsample;
	int pulse_len;
	double pulse_margin;
	double skew;
	int action;
};

// --------------------------------------------------------------------------
void cfg_drop(struct config *cfg)
{
	if (!cfg) return;

	free(cfg->input_name);
	free(cfg->output_name);
	free(cfg);
}

// --------------------------------------------------------------------------
void print_usage()
{
	printf("Virtual 9-Track Tape Drive v%s\n\n", V9TTD_VERSION);
	printf(
	"Usage: v9ttd -i input -S [-c chlist] [-d downsample]\n"
	"       v9ttd -i input -p pulse_len [-c chlist] [-m pulse_margin] [-d downsample] [-s skew]\n"
	"       v9ttd -h\n"
	"\n"
	"Options:\n"
	"   -h        : Print this help\n"
	"   -S        : Calculate and print pulse statistics\n"
	"   -i input  : Input file name\n"
	"   -p len    : Base pulse length (>1)\n"
	"   -c chlist : Input channel list specified as: p,7,6,5,4,3,2,1,0\n"
	"               (p=parity track, 0=LSB track). Default is: 8,7,6,5,4,3,2,1,0\n"
	"   -m margin : Base pulse length margin (0.0-0.5, default %.2f)\n"
	"   -s skew   : Maximum allowed inter-track skew (0.0-0.5, default %.2f)\n"
	"   -d ratio  : Downsample input by ratio (>1)\n"
	, DEFAULT_MARGIN, DEFAULT_SKEW
	);
}

// --------------------------------------------------------------------------
struct config * parse_args(int argc, char **argv)
{
	struct config *cfg = calloc(1, sizeof(struct config));
	if (!cfg) return NULL;

	cfg->action = A_UNKNOWN;
	cfg->pulse_margin = DEFAULT_MARGIN;
	cfg->skew = DEFAULT_SKEW;
	cfg->downsample = 1;
	for (int i=0 ; i<9 ; i++) {
		cfg->chmap[i] = 8-i;
	}

	int option;
	char *ch;
	char *s;
	int chcount = 0;

	while ((option = getopt(argc, argv,"hSi:p:c:m:s:d:")) != -1) {
		switch (option) {
			case 'h':
				cfg->action |= A_HELP;
				break;
			case 'S':
				cfg->action |= A_STATS;
				break;
			case 'i':
				cfg->input_name = strdup(optarg);
				break;
			case 'p':
				cfg->action |= A_ANALYZE;
				cfg->pulse_len = atoi(optarg);
				break;
			case 'c':
				s = optarg;
				while ((ch = strtok(s, ",")) > 0) {
					int chnum = atoi(ch);
					if ((chnum < 0) || (chnum > 15)) {
						printf("Channel numbers allowed in '-c' argument are 0-15\n");
						cfg->action |= A_QUIT;
						break;
					}
					cfg->chmap[chcount] = chnum;
					s = NULL;
					chcount++;
				}
				if ((chcount != 9) && (cfg->action != A_QUIT)) {
					printf("Wrong number of channels for '-c' argument, need exactly 9.\n");
					cfg->action |= A_QUIT;
				}
				break;
			case 'm':
				cfg->action |= A_ANALYZE;
				cfg->pulse_margin = atof(optarg);
				break;
			case 's':
				cfg->action |= A_ANALYZE;
				cfg->skew = atof(optarg);
				break;
			case 'd':
				cfg->downsample = atoi(optarg);
				break;
			default:
				cfg->action |= A_QUIT;
				break;
		}
	}

	return cfg;
}

// --------------------------------------------------------------------------
int check_config(struct config *cfg)
{
	if (!cfg) {
		printf("Something went terribly wrong, there is no configuration...\n");
		return -1;
	} else if ((cfg->action & (A_ANALYZE | A_STATS)) == (A_ANALYZE | A_STATS)) {
		printf("Can't mix '-S' option with analysis options\n");
		return -1;
	} else if ((cfg->input_name) && (cfg->action != A_ANALYZE) && (cfg->action != A_STATS)) {
		printf("Don't know what to do with the input file. Please specify either '-S' or '-p'\n");
		return -1;
	} else if ((cfg->action != A_HELP) && (!cfg->input_name)) {
		printf("Input tape image file name '-i' is required\n");
		return -1;
	} else if ((cfg->action == A_ANALYZE) && (!cfg->pulse_len)) {
		printf("Pulse length '-p' is required\n");
		return -1;
	}

	return 0;
}

// --------------------------------------------------------------------------
int main(int argc, char **argv)
{
	struct config *cfg = parse_args(argc, argv);

	if ((cfg->action & A_QUIT) || check_config(cfg)) {
		cfg_drop(cfg);
		exit(0);
	} else if ((cfg->action & A_HELP)) {
		print_usage();
		cfg_drop(cfg);
		exit(0);
	}

	setbuf(stdout, NULL);

	printf("Loading tape image '%s' with channel order (parity, msb, ..., lsb): ", cfg->input_name);
	for (int i=0 ; i<9 ; i++) {
		printf("%i", cfg->chmap[i]);
		if (i<8) {
			printf(", ");
		}
	}
	printf("\n");

	struct vtape *t = vtape_open(cfg->input_name, cfg->chmap, cfg->downsample);
	printf("Loaded %i samples (downsample factor: 1/%i)\n", t->sample_count, cfg->downsample);

//	struct vtape *t = vtape_make(test_tape, 16, chmap);

	if (cfg->action == A_STATS) {
		printf("Analyzing pulse lengths... ");
		vtape_scan_pulses(t, 1000);
		printf("most frequent pulse is %i samples\n", t->mfp);
		printf("Pulse length histogram:\n");
		print_stats(t);
		vtape_rewind(t);
	} else {
		vtape_set_bpl(t, cfg->pulse_len, cfg->pulse_len * cfg->pulse_margin);
		vtape_set_skew(t, cfg->pulse_len * cfg->skew);

		printf("PE analyzer initialized with: bpl=%i, margin=%i (%.2f), short_pulse=[%i..%i], long_pulse=[%i..%i], skew_max=%i (%.2f)\n", t->bpl, t->bpl_margin, cfg->pulse_margin, t->bpl_min, t->bpl_max, t->bpl2_min, t->bpl2_max, t->skew_max, cfg->skew);

		printf("Running PE analysis... ");
		pe_analyze(t);
		printf("got %i PE blocks and %i PE tape marks\n", t->blocks[F_PE], t->marks[F_PE]);
	}

	vtape_close(t);
	cfg_drop(cfg);

	return 0;
}

// vim: tabstop=4 shiftwidth=4 autoindent
