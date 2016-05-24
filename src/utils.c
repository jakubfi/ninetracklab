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
#include <stdarg.h>

#include "vtape.h"

int enable_debug = 0;

// -----------------------------------------------------------------------
void VTDEBUG(char *format, ...)
{
	if (enable_debug) {
		va_list ap;
		va_start(ap, format);
		vprintf(format, ap);
		va_end(ap);
	}
}

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
int parity9(int x)
{
	for (int d=8 ; d>0 ; d>>=2) {
		x ^= x >> d;
	}
	return x & 1;
}

// vim: tabstop=4 shiftwidth=4 autoindent
