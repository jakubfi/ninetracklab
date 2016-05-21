#!/bin/env python3

import csv
import struct

f = open("data/zcd5-env5_binary_cut.csv", "r")
fo = open("data/zcd5-env5_cut.bin", "wb")

rd = csv.reader(f, delimiter=",")

for i in rd:
    word = int(i[2]) + (int(i[1]) << 1)
    word = struct.pack("<H", word)
    fo.write(word)
