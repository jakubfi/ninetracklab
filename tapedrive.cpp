#include <QProgressDialog>
#include <QDebug>
#include <QTime>

#include "ninetracklab.h"
#include "tapedrive.h"

//static int defchmap[9] = { 7, 6, 5, 4, 3, 2, 1, 0, 8 };

// --------------------------------------------------------------------------
TapeDrive::TapeDrive(QWidget *parent) : nrz1(this), pe(this)
{
	this->parent = parent;
	data = NULL;
	tape = NULL;
	tape_samples = 0;
	pos = 0;
}

// --------------------------------------------------------------------------
int TapeDrive::parity9(quint8 x)
{
	x ^= x >> 8;
	x ^= x >> 4;
	x ^= x >> 2;
	x ^= x >> 1;
	return x & 1;
}

// --------------------------------------------------------------------------
void TapeDrive::remap()
{
	QTime myTimer;
	myTimer.start();

	for (int pos=0 ; pos<30 ; pos++) {
		data[pos] = 0xffff;
	}
	for (int pos=tape_samples-30 ; pos<tape_samples ; pos++) {
		data[pos] = 0xffff;
	}
	for (int pos=30 ; pos<tape_samples-30 ; pos++) {
		for (int ch=0 ; ch<9 ; ch++) {
			quint16 src_d = tape[pos];
			quint16 source_bit = (src_d >> cfg.chmap[ch]) & 1;
			data[pos] |= source_bit << ch;
		}
	}

	int ms = myTimer.elapsed();
	qDebug() << "remap took" << ms << "ms";
}

// --------------------------------------------------------------------------
void TapeDrive::realign()
{
	QTime myTimer;
	myTimer.start();

	int start_pos;

	for (int ch=0 ; ch<9 ; ch++) {
		start_pos = -1;
		for (int pos=1 ; pos<tape_samples ; pos++) {
			int val = data[pos];
			int valp = data[pos-1];
			int row = val ^ valp;

			// found edge
			if ((row >> ch) & 1) {
				int len = pos - start_pos;
				// second edge
				if (len <= cfg.realign_margin) {
					int p;
					for (p=start_pos ; p<pos ; p++) {
						data[p] ^= 1 << ch;
					}
					int new_start = start_pos + cfg.realign_push;
					for (p=new_start ; p<new_start+len ; p++) {
						data[p] ^= 1 << ch;
					}
					start_pos = -1;
					pos = p;
				// new edge
				} else {
					start_pos = pos;
				}
			}
		}
	}

	int ms = myTimer.elapsed();
	qDebug() << "realign took" << ms << "ms";
}

// --------------------------------------------------------------------------
void TapeDrive::deglitch_old()
{
	QTime myTimer;
	myTimer.start();

	int start_pos[9] = {0,0,0,0,0,0,0,0,0};
	int maxlen = cfg.glitch_max;
	if ((maxlen < 1) && (cfg.glitch_single)) maxlen = 1;
	int counter[9];
	qFill(counter, counter+9, maxlen);

	for (int pos=100 ; pos<tape_samples-100 ; pos++) {
		for (int ch=0 ; ch<9 ; ch++) {
			int val = data[pos];
			int valp = data[pos-1];
			int row = val ^ valp;

			// found edge
			if ((row >> ch) & 1) {
				// second edge
				if (counter[ch] > 0) {
					// remove glitches
					if (
							((cfg.glitch_single) && (pos-start_pos[ch] <= 1)) ||
							((pos-start_pos[ch] <= cfg.glitch_max) && (((data[pos-1]>>ch)&1) == ((data[pos+cfg.glitch_distance]>>ch)&1)))
					) {
						for (int p=start_pos[ch] ; p<pos ; p++) {
							data[p] ^= 1 << ch;
						}
					}
					counter[ch] = 0;
				// new edge
				} else {
					counter[ch] = maxlen;
					start_pos[ch] = pos;
				}
			// no edge
			} else {
				// decrease counter if in pulse
				if (counter[ch] > 0) counter[ch]--;
			}
		}
	}

	int ms = myTimer.elapsed();
	qDebug() << "deglitch took" << ms << "ms";
}

// --------------------------------------------------------------------------
void TapeDrive::deglitch_new()
{
	QTime myTimer;
	myTimer.start();

	int start_pos;

	for (int ch=0 ; ch<9 ; ch++) {
		start_pos = -1;
		for (int pos=1 ; pos<tape_samples ; pos++) {
			int val = data[pos];
			int valp = data[pos-1];
			int row = val ^ valp;
			int edge_up = ((row & val) >> ch) & 1;
			int edge_dn = ((row & valp) >> ch) & 1;

			// first edge
			if (edge_dn) {
				start_pos = pos;
			// second edge
			} else if (edge_up && (start_pos != -1)) {
				if (pos-start_pos <= cfg.glitch_max) {
					// remove glitches
					for (int p=start_pos ; p<pos ; p++) {
						data[p] ^= 1 << ch;
					}
				}
				start_pos = -1;
			}
		}
	}

	int ms = myTimer.elapsed();
	qDebug() << "deglitch took" << ms << "ms";
}

// --------------------------------------------------------------------------
int TapeDrive::preprocess(TDConf &c)
{
	if (!tape) {
		return VT_ELOAD;
	}

	if (data) {
		delete[] data;
	}
	data = new quint16[tape_samples]();

	cfg = c;

	remap();
	realign();
	deglitch_new();

	return VT_OK;
}

// --------------------------------------------------------------------------
void TapeDrive::wiggle_wiggle_wiggle(TapeChunk &chunk)
{
	int scatter_initial = getMissalign(chunk);

	for (int ch=0 ; ch<9 ; ch++) {
		int iterations = 0;
		for (int dir=-1 ; dir <=1 ; dir+=2) {
			if (iterations <= 1) {
				forever {
					cfg.unscatter[ch] -= dir;
					iterations++;
					int cscatter = getMissalign(chunk);
					if (cscatter < scatter_initial) {
						scatter_initial = cscatter;
					} else {
						cfg.unscatter[ch] += dir;
						break;
					}
				}
			}
		}
	}
}

// --------------------------------------------------------------------------
int TapeDrive::getMissalign(TapeChunk &chunk, int edges_sample)
{
	int pulse_start = -1;
	int last_pulse_start = 0;
	int bpulses = 0;

	int bpl = 25;
	int center = bpl * 0.4;
	int margin = 6;

	seek(chunk.beg, TD_SEEK_SET);

	while ((edges_sample > 0) && (pulse_start < chunk.end)) {
		int pulse = read(&pulse_start, 0, EDGE_RISING);
		if (pulse < 0) break;
		int delta = pulse_start-last_pulse_start;
		if ((delta >= center-margin) && (delta <= center+margin)) {
			bpulses++;
		}
		last_pulse_start = pulse_start;
		edges_sample--;
	}

	return bpulses;
}

/*
// --------------------------------------------------------------------------
int TapeDrive::read_bunch(int *pulses_start, int *first, int *last, int deskew_max, int edge, int dir)
{
	int pulse = 0;
	int got_start = 0;
	qFill(pulses_start, pulses_start+9, -1);

	do {
		int tpulse = get_edge(pos, edge, dir);
		if (tpulse < 0) {
			return tpulse;
		}

		if (!got_start) {
			if (tpulse) {
				got_start = 1;
				*first = pos;
			}
		} else {
			deskew_max--;
		}

		for (int ch=0 ; ch<9 ; ch++) {
			if (((tpulse>>ch)&1) && (pulses_start[ch] == -1)) {
				pulses_start[ch] = pos;
			}
		}

		pulse |= tpulse;
		pos += dir;
	} while (!pulse || (deskew_max > 0));

	*last = pos;

	return pulse;
}

// --------------------------------------------------------------------------
void TapeDrive::unscatter_old(TapeChunk &chunk)
{
	int pulses_start[9];
	EdgeSens edge;
	int unscatter_done = 0;
	int reference_start = 0;
	int last, first;
	int pulse;
	int fixing_rows = 2;

	// tape is going to be read backwards - reverse edges
	if (cfg.edge_sens == EDGE_FALLING) {
		edge = EDGE_RISING;
	} else if (chunk.cfg.edge_sens == EDGE_RISING) {
		edge = EDGE_FALLING;
	} else {
		edge = EDGE_ANY;
	}

	// reset current unscatter
	seek(chunk.end, TD_SEEK_SET);
	qFill(cfg.unscatter, cfg.unscatter+9, 0);
	cfg.unscatter_fixed = 0;

	while (unscatter_done != 0b111111111) {
		reference_start = 0;
		pulse = read_bunch(pulses_start, &first, &last, cfg.bpl*0.8, edge, DIR_BACKWARD);
		if (pulse < 0) {
			return;
		}
		if ((cfg.unscatter_fixed & pulse)) {
			for (int ch=0 ; ch<9 ; ch++) {
				if ((pulses_start[ch] > -1) && ((cfg.unscatter_fixed>>ch)&1)) {
					reference_start = pulses_start[ch];
					pulses_start[ch] = -1;
					//qDebug() << "found reference" << reference_start << "to fixed track" << ch;
					break;
				}
			}
		} else {
			reference_start = first;
			//qDebug() << "using first edge for reference" << reference_start;
		}
		for (int ch=0 ; ch<9 ; ch++) {
			if ((pulses_start[ch] > -1) && !((unscatter_done>>ch)&1)) {
				//qDebug() << "unscatter for" << ch << "is now" << reference_start - pulses_start[ch];
				cfg.unscatter[ch] = reference_start - pulses_start[ch];
			}
		}
		unscatter_done |= pulse;
		if (fixing_rows > 0) {
			cfg.unscatter_fixed |= pulse;
			fixing_rows--;
		}
	}
}
*/

// --------------------------------------------------------------------------
void TapeDrive::unscatter(TapeChunk &chunk)
{
	int pulse_start = 0;
	int reference_start = 0;
	quint16 unscatter_done;
	int group = 0;

	seek(chunk.end, TD_SEEK_SET);
	unscatter_done = 0;
	qFill(cfg.unscatter, cfg.unscatter+9, 0);
	cfg.unscatter_fixed = 0;

	int margin = cfg.bpl * 0.85;

	while (unscatter_done != 0b111111111) {
		int pulse = read(&pulse_start, 0, EDGE_FALLING, DIR_BACKWARD);
		if (pulse < 0) {
			break;
		}

		// new reference
		if ((!unscatter_done) || (pulse & unscatter_done)) {
			for (int ch=0 ; ch<9 ; ch++) {
				if ((pulse>>ch)&1) {
					unscatter_done |= 1 << ch;
				}
			}
			reference_start = pulse_start;
			group++;
		// pulses within
		} else {
			int delta = reference_start - pulse_start;
			if (delta <= margin) {
				for (int ch=0 ; ch<9 ; ch++) {
					if ((pulse>>ch)&1) {
						cfg.unscatter[ch] = delta;
						unscatter_done |= 1 << ch;
					}
				}
			}
		}
		if (group <= 2) {
			cfg.unscatter_fixed |= unscatter_done;
		}

	}
}

// --------------------------------------------------------------------------
int TapeDrive::load(QString filename)
{
	unload();

	tape_file.setFileName(filename);
	tape_file.open(QFile::ReadOnly);
	tape = (quint16*) tape_file.map(0, tape_file.size());
	tape_samples = tape_file.size() / 2;

	return 0;
}

// --------------------------------------------------------------------------
int TapeDrive::unload()
{
	if (tape) {
		tape_file.unmap((uchar*)tape);
		tape_file.close();
		tape = NULL;
	}

	delete[] data;
	data = NULL;

	tape_samples = 0;

	return 0;
}

// --------------------------------------------------------------------------
void TapeDrive::exportCut(QString filename, long left, long right)
{
	QFile out(filename);
	out.open(QIODevice::WriteOnly);
	out.write((const char*)tape+2*left, 2*(right-left));
	out.close();
}

// --------------------------------------------------------------------------
int TapeDrive::peek(int p)
{
	int pulse = 0;
	for (int ch=0 ; ch<9 ; ch++) {
		int uspos = p - cfg.unscatter[ch];
		if (uspos < 0) {
			uspos = 0;
		} else if (uspos > tape_samples) {
			return VT_EOT;
		}
		int tpulse = data[uspos];
		pulse |= tpulse & (1<<ch);
	}
	return pulse;
}

// --------------------------------------------------------------------------
int TapeDrive::get_edge_internal(int p, int edge, int dir)
{
	if (dir == DIR_FORWARD) {
		if (p < 0) {
			p = 0;
		} else if (p+1 >= tape_samples) {
			return VT_EOT;
		}
	} else {
		if (p-1 < 0) {
			return VT_EOT;
		} else if (p >= tape_samples) {
			p = tape_samples;
		}
	}

	int pulse = data[p] ^ data[p+dir];
	if (edge == EDGE_FALLING) {
		pulse &= data[p];
	} else if (edge == EDGE_RISING) {
		pulse &= data[p+dir];
	}

	return pulse;
}

// --------------------------------------------------------------------------
int TapeDrive::get_edge(int p, int edge, int dir)
{
	int pulse = 0;

	for (int ch=0 ; ch<9 ; ch++) {
		int uspos = p - cfg.unscatter[ch];
		int tpulse = get_edge_internal(uspos, edge, dir);
		if (tpulse < 0) {
			return tpulse;
		} else if (tpulse) {
			pulse |= tpulse & (1<<ch);
		}
	}

	return pulse;
}

// --------------------------------------------------------------------------
int TapeDrive::read(int *pulse_start, int deskew_max, int edge, int dir)
{
	int pulse = 0;
	int got_start = 0;

	do {
		int tpulse = get_edge(pos, edge, dir);
		if (tpulse < 0) {
			return tpulse;
		}

		if (!got_start) {
			if (tpulse) {
				got_start = 1;
				*pulse_start = pos;
			}
		} else {
			deskew_max--;
		}

		pulse |= tpulse;
		pos += dir;
	} while (!pulse || (deskew_max > 0));

	return pulse;
}

// --------------------------------------------------------------------------
int TapeDrive::seek(unsigned long offset, int whence)
{
	switch (whence) {
	case TD_SEEK_CUR:
		pos += offset;
		break;
	case TD_SEEK_END:
		pos = tape_samples - 1 - offset;
		break;
	case TD_SEEK_SET:
		pos = offset;
		break;
	default:
		return VT_EOT;
	}
	return VT_OK;
}

// --------------------------------------------------------------------------
TapeChunk TapeDrive::scan_next_chunk(int start)
{
	return nrz1.scan_next_chunk(start);
}

#define PROCESS_OK ((chunk.vpar_err_count == 0) && (chunk.crc_tape == chunk.crc_data) && (chunk.hpar_tape == chunk.hpar_data))
//#define PROCESS_ERR_SMALL ((chunk.bytes > 256) && ((chunk.vpar_err_count*3) < chunk.bytes))
#define PROCESS_ERR_SMALL (chunk.vpar_err_count*100/chunk.bytes < 20)

struct sc {
	int val;
	int pos;
};

// --------------------------------------------------------------------------
bool sc_lessthan(const sc &s1, const sc &s2)
{
	return qAbs(s1.val) > qAbs(s2.val);
}
#define QT_NO_DEBUG_OUTPUT 1
// --------------------------------------------------------------------------
int TapeDrive::process(TapeChunk &chunk)
{
	int best_deskew, init_deskew;
	int least_errors;
	cfg = chunk.cfg;

	int permutable;
	QList<sc> lu;
	int usc[9];
	int permutations;
	int cnt = 0;

	QTime myTimer;
	myTimer.start();

	/* --- TRY AS IS ------------------------------------------------------ */
	qDebug() << "Initial deskew:" << cfg.deskew;
	qDebug() << "Initial unscatter: " << cfg.unscatter[8] << cfg.unscatter[7] << cfg.unscatter[6] << cfg.unscatter[5] << cfg.unscatter[4] << cfg.unscatter[3] << cfg.unscatter[2] << cfg.unscatter[1] << cfg.unscatter[0];
	nrz1.process(chunk);
	qDebug() << "Verrors: " << chunk.vpar_err_count << "(" << chunk.vpar_err_count*100/(chunk.bytes+1) << "%) Herrors:" << chunk.hpar_err << "CRC ok?" << (chunk.crc_tape == chunk.crc_data);

	if (PROCESS_OK) goto fin;
	if (PROCESS_ERR_SMALL) goto small;

// huge:

	/* --- UNSCATTER + WIGGLE --------------------------------------------- */
	if (cfg.unscatter_auto) {
		unscatter(chunk);
		qDebug() << "After unscatter: " << cfg.unscatter[8] << cfg.unscatter[7] << cfg.unscatter[6] << cfg.unscatter[5] << cfg.unscatter[4] << cfg.unscatter[3] << cfg.unscatter[2] << cfg.unscatter[1] << cfg.unscatter[0];
		wiggle_wiggle_wiggle(chunk);
		qDebug() << "After wiggle: " << cfg.unscatter[8] << cfg.unscatter[7] << cfg.unscatter[6] << cfg.unscatter[5] << cfg.unscatter[4] << cfg.unscatter[3] << cfg.unscatter[2] << cfg.unscatter[1] << cfg.unscatter[0];
		nrz1.process(chunk);
		qDebug() << "Verrors: " << chunk.vpar_err_count << "(" << chunk.vpar_err_count*100/(chunk.bytes+1) << "%) Herrors:" << chunk.hpar_err << "CRC ok?" << (chunk.crc_tape == chunk.crc_data);
		if (PROCESS_OK) goto fin;
		if (PROCESS_ERR_SMALL) goto small;
	}

	/* --- PERMUTATE ------------------------------------------------------ */
	permutable = ~cfg.unscatter_fixed & 0b111111111;

	qCopy(cfg.unscatter, cfg.unscatter+9, usc);
	for (int i=0 ; i<9 ; i++) {
		if (((permutable >> i) & 1) && (usc[i] != 0)) {
			sc x = {usc[i], i};
			lu << x;
		}
	}
	qSort(lu.begin(), lu.end(), sc_lessthan);
	permutations = (1 << lu.count()) - 1;
	for (int i=1 ; i<=permutations ; i++) {
		qCopy(usc, usc+9, cfg.unscatter);
		for (int j=0 ; j<lu.count() ; j++) {
			if ((i>>j)&1) {
				if (lu[j].val <= 0) {
					cfg.unscatter[lu[j].pos] += 25;
				} else {
					cfg.unscatter[lu[j].pos] -= 25;
				}
			}
		}
		qDebug() << "Permutation" << cnt << " : " << cfg.unscatter[8] << cfg.unscatter[7] << cfg.unscatter[6] << cfg.unscatter[5] << cfg.unscatter[4] << cfg.unscatter[3] << cfg.unscatter[2] << cfg.unscatter[1] << cfg.unscatter[0];
		cnt++;
		nrz1.process(chunk);
		qDebug() << "Verrors: " << chunk.vpar_err_count << "(" << chunk.vpar_err_count*100/(chunk.bytes+1) << "%) Herrors:" << chunk.hpar_err << "CRC ok?" << (chunk.crc_tape == chunk.crc_data);
		if (PROCESS_OK) goto fin;
		if (PROCESS_ERR_SMALL) goto small;
	}

small:

	/* --- DESKEW -------------------------------------------------- */
	best_deskew = init_deskew = cfg.deskew;
	least_errors = chunk.vpar_err_count;
	if (cfg.deskew_auto) {
		for (int dir=1 ; dir>=-1 ; dir-=2) {
			for (int deskew=init_deskew ; (deskew>=cfg.bpl*0.2) && (deskew<=cfg.bpl*0.8) ; deskew+=dir) {
				cfg.deskew = deskew;
				qDebug() << "New deskew:" << cfg.deskew;
				nrz1.process(chunk);
				qDebug() << "Verrors: " << chunk.vpar_err_count << "(" << chunk.vpar_err_count*100/(chunk.bytes+1) << "%) Herrors:" << chunk.hpar_err << "CRC ok?" << (chunk.crc_tape == chunk.crc_data);
				if (PROCESS_OK) {
					goto fin;
				} else if (chunk.vpar_err_count <= least_errors) {
					least_errors = chunk.vpar_err_count;
					best_deskew = cfg.deskew;
				} else {
					break;
				}
			}
			init_deskew--;
		}
		qDebug() << "Using best deskew:" << best_deskew;
		cfg.deskew = best_deskew;
	}

	if (chunk.type != C_BLOCK) goto fin;

	/* --- WIGGLE ----------------------------------------------- */
	if (cfg.unscatter_auto) {
		wiggle_wiggle_wiggle(chunk);
		qDebug() << "After wiggle: " << cfg.unscatter[8] << cfg.unscatter[7] << cfg.unscatter[6] << cfg.unscatter[5] << cfg.unscatter[4] << cfg.unscatter[3] << cfg.unscatter[2] << cfg.unscatter[1] << cfg.unscatter[0];
		nrz1.process(chunk);
		qDebug() << "Verrors: " << chunk.vpar_err_count << "(" << chunk.vpar_err_count*100/(chunk.bytes+1) << "%) Herrors:" << chunk.hpar_err << "CRC ok?" << (chunk.crc_tape == chunk.crc_data);
	}

	/* --- DESKEW -------------------------------------------------- */
	best_deskew = init_deskew = cfg.deskew;
	least_errors = chunk.vpar_err_count;
	if (cfg.deskew_auto) {
		for (int dir=-1 ; dir<=1 ; dir+=2) {
			for (int deskew=init_deskew ; (deskew>=cfg.bpl*0.2) && (deskew<=cfg.bpl*0.8) ; deskew+=dir) {
				cfg.deskew = deskew;
				qDebug() << "New deskew:" << cfg.deskew;
				nrz1.process(chunk);
				qDebug() << "Verrors: " << chunk.vpar_err_count << "(" << chunk.vpar_err_count*100/(chunk.bytes+1) << "%) Herrors:" << chunk.hpar_err << "CRC ok?" << (chunk.crc_tape == chunk.crc_data);
				if (PROCESS_OK) {
					goto fin;
				} else if ((chunk.vpar_err_count <= least_errors) || (chunk.vpar_err_count == chunk.hpar_err_count)) {
					least_errors = chunk.vpar_err_count;
					best_deskew = cfg.deskew;
				} else {
					cfg.deskew -= dir;
					break;
				}
			}
			init_deskew--;
		}
		cfg.deskew = best_deskew;
		qDebug() << "Using best deskew:" << best_deskew;
		if (!(PROCESS_OK)) {
			nrz1.process(chunk);
		}
	}

fin:

	bitFix(chunk);

	chunk.cfg = cfg;

	int ms = myTimer.elapsed();
	qDebug() << "Final Verrors: " << chunk.vpar_err_count << "(" << chunk.vpar_err_count*100/(chunk.bytes+1) << "%) Herrors:" << chunk.hpar_err << "CRC ok?" << (chunk.crc_tape == chunk.crc_data);

	qDebug() << "fin: " << ms << " ms";
	return VT_OK;
}

// --------------------------------------------------------------------------
void TapeDrive::bitFix(TapeChunk &chunk)
{
	if (chunk.vpar_err_count != 1) return;
	if (chunk.hpar_err_count != 1) return;

	for (int i=0 ; i<chunk.bytes ; i++) {
		int vp = !(parity9(chunk.data[i]) ^ (chunk.data[i]>>8));
		if (vp) {
			chunk.data[i] ^= chunk.hpar_err;
			qDebug() << "Fixed one-bit parity error";
		}
	}

	chunk.crc_data = nrz1.crc(chunk.data, chunk.bytes);
	if (chunk.crc_data == chunk.crc_tape) chunk.fixed = 1;
}

// vim: tabstop=4 shiftwidth=4 autoindent
