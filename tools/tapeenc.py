#!/bin/env python3

import sys
import random

PE = 1
NRZ1 = 2

pulse_len = 60
blk_size = 64
presize = 40
postsize = 40
mark = 0b11111111
marksize = 100
encoder = NRZ1

nrz1_data = 0

# ------------------------------------------------------------------------
def parity9(x):
    x ^= x >> 8
    x ^= x >> 4
    x ^= x >> 2
    x ^= x >> 1
    return (~x) & 1

# ------------------------------------------------------------------------
def write_signal(outf, pmin, pmax, samples, zero=False):
    print("  signal: %i * [%i..%i]" % (samples, pmin, pmax))
    for i in range(samples):
        if encoder == NRZ1:
            a = nrz1_data & 0xff
            b = nrz1_data >> 8
        else:
            a = int(random.random()*0xff)
            b = int(random.random()*0xff)
        outf.write(bytes([a, b&1]))

# ------------------------------------------------------------------------
def write_row_nrz1(outf, data, parity):
    print("  data: %i (%c), parity: %i" % (data, data, parity))

    global nrz1_data

    d = bytes([nrz1_data&0xff, nrz1_data>>8])
    for i in range(pulse_len>>1):
        outf.write(d)

    nrz1_data ^= ((data&0xff) | (parity<<8))

    d = bytes([nrz1_data&0xff, nrz1_data>>8])
    for i in range(pulse_len>>1):
        outf.write(d)

# ------------------------------------------------------------------------
def write_row_pe(outf, data, parity):
    #print("  data: %i (%c), parity: %i" % (data, data, parity))
    for i in range(pulse_len):
        outf.write(bytes([data, parity&1]))
    for i in range(pulse_len):
        outf.write(bytes([(~data)&0xff, (~parity)&1]))

# ------------------------------------------------------------------------
def write_preamble(outf):
    print(" Preamble, %i repetitions" % (presize))
    for i in range(presize):
        row_writer(outf, 0, 0)
    row_writer(outf, 0xff, 1)

# ------------------------------------------------------------------------
def write_postamble(outf):
    print(" Postamble, %i repetitions" % (postsize))
    row_writer(outf, 0xff, 1)
    for i in range(postsize):
        row_writer(outf, 0, 0)

# ------------------------------------------------------------------------
def write_mark(outf):
    print(" Mark %i, %i repetitions" % (mark, marksize))
    if encoder == PE:
        for i in range(marksize):
            row_writer(outf, mark, 0)
    else:
        row_writer(outf, 0b10100100, 0)
        row_writer(outf, 0, 0)
        row_writer(outf, 0, 0)
        row_writer(outf, 0, 0)
        row_writer(outf, 0, 0)
        row_writer(outf, 0, 0)
        row_writer(outf, 0, 0)
        row_writer(outf, 0, 0)
        row_writer(outf, 0b10100100, 0)


# ------------------------------------------------------------------------
def write_data(outf, pulse_len, data):
    print(" Data: %s" % data)
    for d in data:
        parity = parity9(d)
        row_writer(outf, d, parity)
    row_writer(outf, 0, 0)
    row_writer(outf, 0, 0)
    row_writer(outf, 0, 0)
    row_writer(outf, 0xde, 0)
    row_writer(outf, 0, 0)
    row_writer(outf, 0, 0)
    row_writer(outf, 0, 0)
    row_writer(outf, 0xad, 0)

# ------------------------------------------------------------------------
def write_file(inf, outf):
    data = inf.read(blk_size)
    while len(data) > 0:
        if encoder == PE:
            write_preamble(outf)
        write_data(outf, pulse_len, data)
        if encoder == PE:
            write_postamble(outf)
        data = inf.read(blk_size)
    if encoder == PE:
        write_signal(outf, 0, 0, 100)
    else:
        write_signal(outf, 0, 0, 100, zero=True)

    write_mark(outf)

# ------------------------------------------------------------------------
def write_tape(input_files, output_file):
    outf = open(output_file, "wb+")
    print("Creating image file: %s" % output_file)

    if encoder == PE:
        z = False
    else:
        z = True

    write_signal(outf, 0, 0, 100, zero=z);

    for f in input_files:
        print("Saving file: %s" % f)
        inf = open(f, "rb")
        write_file(inf, outf)
        inf.close()

    write_signal(outf, 0, 0, 100, zero=z)
    write_mark(outf)
    write_signal(outf, 0, 0, 100, zero=z);

    outf.close()

# ------------------------------------------------------------------------
# --- MAIN ---------------------------------------------------------------
# ------------------------------------------------------------------------

# Usage: tapenc.py out_image in_file in_file ...

if encoder == PE:
    row_writer = write_row_pe
else:
    row_writer = write_row_nrz1


output_file = sys.argv[1]

write_tape(sys.argv[2:], output_file)

# vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4
