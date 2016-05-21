from pstage import *
import numpy as np
import os
import resource

# ------------------------------------------------------------------------
class FilterDownsample(PStage):

    # ------------------------------------------------------------------------
    def __init__(self, prev, div):
        PStage.__init__(self, prev, PStage.FILTER, prev.otype, prev.otype)
        self.name = "Downsampler"
        self.div = div

    # --------------------------------------------------------------------
    def get_row_count(self):
        return int(self.prev.row_count / self.div)

    # --------------------------------------------------------------------
    def prepare_row(self):
        self.l("prepare_row()")
        row = self.prev.read()
        cnt = self.div
        while cnt > 1:
            self.prev.read()
            cnt -= 1
        return row

# ------------------------------------------------------------------------
class FilterAmpStats(PStage):

    # ------------------------------------------------------------------------
    def __init__(self, prev):
        PStage.__init__(self, prev, PStage.SINK, Data.ROW_LINEAR, Data.NONE)
        self.name = "AmpStats"
        self.do_stats()

    # ------------------------------------------------------------------------
    def do_stats(self):
        count = self.prev.get_row_count()
        print("Total samples: %i" % count)
        self.prev.peek(int(count/10))
        print("%i MB" % int(resource.getrusage(resource.RUSAGE_SELF).ru_maxrss/1024))
        return
        tracks = np.flipud(np.rot90(self.prev.buf))
        screen_width = int(os.popen('stty size', 'r').read().split()[1])

        for trackn, t in enumerate(tracks):
            print("Track %i amplitude stats (min: %f, max: %f, avg: %f, med: %f)" % (trackn, t.min(), t.max(), np.average(t), np.median(t)))
            hist, edges = np.histogram(t, bins=10)
            for i, e in enumerate(edges):
                if i > 0:
                    hrange = "%.2f ... %.2f" % (edges[i-1], e)
                else:
                    hrange = "< %.2f" % e
                val = np.ceil((screen_width-30) * (hist[i-1] / np.max(hist)))
                print("%18s : %s" % (hrange, "#" * val))


# vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4
