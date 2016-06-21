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

#ifndef __UTILS_H__
#define __UTILS_H__

#include <inttypes.h>

#include "vtape.h"

void VTDEBUG_ON();
void VTDEBUG(int level, char *format, ...);
int term_width();
void print_stats(struct vtape *t);
int parity9(uint8_t x);

#endif

// vim: tabstop=4 shiftwidth=4 autoindent
