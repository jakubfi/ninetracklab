#!/bin/env python3

import csv
import math
import struct
import numpy as np

from pstage import *

# ------------------------------------------------------------------------
class PStageReader(PStage):

    # ------------------------------------------------------------------------
    def get_line_count(self, f):
        orig_pos = f.tell()
        i = -1
        for i, l in enumerate(f):
            pass
        f.seek(orig_pos)
        return i + 1

    # --------------------------------------------------------------------
    def get_file_size(self, f):
        orig_pos = f.tell()
        f.seek(0, 2)
        filesize = f.tell()
        f.seek(orig_pos)
        return filesize

    # --------------------------------------------------------------------
    def __init__(self, filename, otype, mode, track_order):
        PStage.__init__(self, None, PStage.INPUT, None, otype)
        self.filename = filename
        self.f = open(filename, mode)
        self.track_order = track_order
        self.file_lines = self.get_line_count(self.f)
        self.file_size = self.get_file_size(self.f)
        self.row_count = 0
        self.tracks = len(track_order)

    # --------------------------------------------------------------------
    def get_row_count(self):
        return self.row_count

    # --------------------------------------------------------------------
    def __del__(self):
        self.f.close()

# ------------------------------------------------------------------------
class ReaderBin(PStageReader):

    endianess_to_struct = { "le" : "<", "be" : ">"}
    bits_to_struct = { 8 : "B", 16 : "H"}

    # --------------------------------------------------------------------
    def __init__(self, filename, bits, endianess, track_order):
        PStageReader.__init__(self, filename, Data.ROW_BINARY, "rb", track_order)
        self.name = "ReaderBin";

        # prepare integer format for unpack()
        self.bytes_per_row = int(math.ceil(bits/8.0))
        self.format = self.endianess_to_struct[endianess] + self.bits_to_struct[bits]

        # get row count
        self.row_count = int(self.file_size / float(self.bytes_per_row))

    # --------------------------------------------------------------------
    def prepare_row(self):
        self.l("prepare_row()")
        row = struct.unpack(self.format, self.f.read(self.bytes_per_row))[0]
        return [((row>>x)&1) for x in self.track_order]

# ------------------------------------------------------------------------
class ReaderCSV(PStageReader):

    # --------------------------------------------------------------------
    def __init__(self, filename, track_order):
        PStageReader.__init__(self, filename, Data.ROW_LINEAR, "r", track_order)
        self.name = "ReaderCSV";

        # get row count
        self.row_count = self.file_lines

        # get CSV dialect
        sniffer = csv.Sniffer()
        dialect = csv.register_dialect("local", sniffer.sniff(self.f.read(1024)))
        self.f.seek(0)

        # open CSV
        self.rd = csv.reader(self.f, dialect="local")

    # --------------------------------------------------------------------
    def prepare_row(self):
        self.l("prepare_row()")
        row = self.rd.__next__()
        return [float(row[x]) for x in self.track_order]


# vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4
