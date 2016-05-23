#!/bin/env python3

import sys
import random

start_a = 0b11111111
start_b = 0b00000001

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
def write_row(outf, pulse_len, data, parity):
    print("  data: %i (%c), parity: %i" % (data, data, parity))
    for i in range(pulse_len):
        outf.write(bytes([data, parity&1]))
    for i in range(pulse_len):
        outf.write(bytes([(~data)&0xff, (~parity)&1]))

# ------------------------------------------------------------------------
def write_preamble(outf, pulse_len, plen):
    print(" Preamble, %i repetitions" % (plen))
    for i in range(plen):
        write_row(outf, pulse_len, 0, 0)
    write_row(outf, pulse_len, 0xff, 1)

# ------------------------------------------------------------------------
def write_postamble(outf, pulse_len, plen):
    print(" Postamble, %i repetitions" % (plen))
    write_row(outf, pulse_len, 0xff, 1)
    for i in range(plen):
        write_row(outf, pulse_len, 0, 0)

# ------------------------------------------------------------------------
def write_mark(outf, pulse_len, mark, marksize):
    print(" Mark %i, %i repetitions" % (mark, marksize))
    for i in range(marksize):
        write_row(outf, pulse_len, mark, 0)

# ------------------------------------------------------------------------
def write_data(outf, pulse_len, data):
    print(" Data: %s" % data)
    for d in data:
        parity = parity9(d)
        write_row(outf, pulse_len, d, parity)

# ------------------------------------------------------------------------
def write_file(inf, outf, pulse_len, blksize, presize, postsize, mark, marksize):
    data = inf.read(blksize)
    while len(data) > 0:
        write_preamble(outf, pulse_len, presize)
        write_data(outf, pulse_len, data)
        write_postamble(outf, pulse_len, postsize)
        data = inf.read(blksize)
    write_signal(outf, 0, 0, 100)
    write_mark(outf, pulse_len, mark, marksize)

# ------------------------------------------------------------------------
def write_tape(input_files, output_file, pulse_len, blksize, presize, postsize, mark, marksize):
    outf = open(output_file, "wb+")
    print("Creating image file: %s" % output_file)
    write_signal(outf, 0, 0, 100);

    for f in input_files:
        print("Saving file: %s" % f)
        inf = open(f, "rb")
        write_file(inf, outf, pulse_len, blksize, presize, postsize, mark, marksize)
        inf.close()

    write_signal(outf, 0, 0, 100)
    write_mark(outf, pulse_len, mark, marksize)
    write_signal(outf, 0, 0, 100);

    outf.close()

# ------------------------------------------------------------------------

output_file = sys.argv[1]

write_tape(sys.argv[2:], output_file, 60, 64, 40, 40, 0b11111111, 100)

# vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4
