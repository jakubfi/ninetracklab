#!/bin/env python3

enable_logging = 0

# ------------------------------------------------------------------------
class Data:

    NONE = 0
    ROW_LINEAR = 1
    ROW_BINARY = 2
    BLOCK = 3

# ------------------------------------------------------------------------
class PStage:

    INPUT = 1
    OUTPUT = 2
    FILTER = 3
    SINK = 4

    # --------------------------------------------------------------------
    def l(self, text):
        if enable_logging:
            print("   %s::%s" % (self.name, text))

    # --------------------------------------------------------------------
    def __init__(self, prev, stype, itype, otype):
        self.stype = stype
        self.itype = itype
        self.otype = otype
        self.prev = prev
        self.rownum = 0
        self.buf = []
        self.name = "???"
        if self.prev:
            self.tracks = self.prev.tracks

    # --------------------------------------------------------------------
    def get_row_count(self):
        # thus needs to be defined for each subclass
        return prev.get_row_count()

    # --------------------------------------------------------------------
    def prepare_row(self):
        # this needs to be defined for each subclass
        return prev.read()

    # --------------------------------------------------------------------
    def buf_fill(self, count):
        self.l("buf_fill(%i)" % count)
        while count > len(self.buf):
            row = self.prepare_row()
            self.buf.append(row)

    # --------------------------------------------------------------------
    def peek(self, count):
        self.l("peek(%i)" % count)
        self.buf_fill(count)
        return self.buf[:count]

    # --------------------------------------------------------------------
    def read(self):
        self.l("read()")
        self.buf_fill(1)
        self.rownum += 1
        return self.buf.pop(0)

    # --------------------------------------------------------------------
    def get_row_num(self):
        return self.rownum


# vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4
