#!/bin/env python3

import os
import os.path
import glob

fcount = 0

# --------------------------------------------------------------------
def new_file_named(f):
    fin = open(f, "rb")
    nr = int(fin.read(1)[0]) << 8
    nr += int(fin.read(1)[0])
    rok = int(fin.read(1)[0]) << 8
    rok += int(fin.read(1)[0])
    miesiac = int(fin.read(1)[0])
    dzien = int(fin.read(1)[0])
    d = fin.read(6).decode("utf-8").strip()
    f = fin.read(6).decode("utf-8").strip()
    typ = int(fin.read(1)[0])

    print("Header: %i (%i-%i-%i) %s %s %i" % (nr, rok, miesiac, dzien, d, f, typ))

    if not os.path.exists(d):
        os.makedirs(d)
    fver = 1
    path = "%s/%s" % (d, f)
    while os.path.isfile(path):
        path = "%s/%s_%02i" % (d, f, fver)
        fver += 1
    fin.close()
    return path

# --------------------------------------------------------------------
def new_file_dummy():
    global fcount
    d = "."
    f = "file_%04i" % fcount
    fcount += 1
    path = "%s/%s" % (d, f)
    return path

# --------------------------------------------------------------------
# --- MAIN -----------------------------------------------------------
# --------------------------------------------------------------------

fout = None

files = glob.glob('blk_*')
for f in sorted(files):

    if f.endswith(".mrk"):
        if fout is not None:
            print("Wrote: %s, %i bytes" % (path, fout.tell()))
            fout.close()
        fout = None
        path = new_file_dummy()

    elif f.endswith(".blk"):
        statinfo = os.stat(f)
        if statinfo.st_size == 32:
            path = new_file_named(f)
        else:
            if fout is None:
                fout = open(path, "wb")
            fin = open(f, "rb")
            data = fin.read()
            fin.close()
            fout.write(data)



