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

#ifndef __NRZ1_H__
#define __NRZ1_H__

#include <inttypes.h>

#include "vtape.h"

uint16_t nrz1_crc(uint16_t *data, int size);
int nrz1_get_block(struct vtape *t, uint16_t *buf);
int nrz1_analyze(struct vtape *t);

#endif

// vim: tabstop=4 shiftwidth=4 autoindent
