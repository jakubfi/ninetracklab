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
encoder = PE

nrze_data = 0

# ------------------------------------------------------------------------
def parity9(x):
    x ^= x >> 8
    x ^= x >> 4
    x ^= x >> 2
    x ^= x >> 1
    return (~x) & 1

# ------------------------------------------------------------------------
def write_signal(outf, pmin, pmax, samples):
    print("  signal: %i * [%i..%i]" % (samples, pmin, pmax))
    for i in range(samples):
        a = int(random.random()*0xff)
        b = int(random.random()*0xff)
        outf.write(bytes([a, b&1]))

# ------------------------------------------------------------------------
def write_row_nrz1(outf, data, parity):
    d = bytes([nrze_data&0xff, nrze_data>>8])
    for i in range(pulse_len/2):
        outf.write(d)
    nrze_data ^= ((data&0xff)<<8) | (parity&1)
    d = bytes([nrze_data&0xff, nrze_data>>8])
    for i in range(pulse_len/2):
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
    for i in range(marksize):
        row_writer(outf, mark, 0)

# ------------------------------------------------------------------------
def write_data(outf, pulse_len, data):
    print(" Data: %s" % data)
    for d in data:
        parity = parity9(d)
        row_writer(outf, d, parity)

# ------------------------------------------------------------------------
def write_file(inf, outf):
    data = inf.read(blk_size)
    while len(data) > 0:
        write_preamble(outf)
        write_data(outf, pulse_len, data)
        write_postamble(outf)
        data = inf.read(blk_size)
    write_signal(outf, 0, 0, 100)
    write_mark(outf)

# ------------------------------------------------------------------------
def write_tape(input_files, output_file):
    outf = open(output_file, "wb+")
    print("Creating image file: %s" % output_file)
    write_signal(outf, 0, 0, 100);

    for f in input_files:
        print("Saving file: %s" % f)
        inf = open(f, "rb")
        write_file(inf, outf)
        inf.close()

    write_signal(outf, 0, 0, 100)
    write_mark(outf)
    write_signal(outf, 0, 0, 100);

    outf.close()

# ------------------------------------------------------------------------

if encoder == PE:
    row_writer = write_row_pe
else:
    row_writer = write_row_nrz1


output_file = sys.argv[1]

write_tape(sys.argv[2:], output_file)

# vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4
