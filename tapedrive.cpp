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

//	qCopy(defchmap, defchmap+9, chmap);
//	qFill(scatter, scatter+9, 0);
//	scatter_fixed = 0;

	//realign_margin = 6;
	//realign_push = 5;
	//glitch_max = 1;
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

// --------------------------------------------------------------------------
void TapeDrive::unscatter(TapeChunk &chunk)
{
	int pulse_start = 0;
	int reference_start = 0;
	quint16 scatter_fixed;

	seek(chunk.end, TD_SEEK_SET);
	scatter_fixed = 0;
	qFill(cfg.unscatter, cfg.unscatter+9, 0);

	int bpl = 25;
	int margin = bpl * 0.15;

	int pulse = read(&pulse_start, 0, EDGE_FALLING, DIR_BACKWARD);
	if (pulse < 0) {
		return;
	}
	for (int ch=0 ; ch<9 ; ch++) {
		if ((pulse>>ch)&1) {
			scatter_fixed |= 1 << ch;
		}
	}
	reference_start = pulse_start;

	while (scatter_fixed != 0b111111111) {
		int pulse = read(&pulse_start, 0, EDGE_FALLING, DIR_BACKWARD);
		if (pulse < 0) {
			break;
		}

		// new reference
		if (pulse & scatter_fixed) {
			for (int ch=0 ; ch<9 ; ch++) {
				if ((pulse>>ch)&1) {
					scatter_fixed |= 1 << ch;
				}
			}
			reference_start = pulse_start;
		} else {
			int delta = reference_start - pulse_start;
			if (delta <= 25-margin) {
				for (int ch=0 ; ch<9 ; ch++) {
					if ((pulse>>ch)&1) {
						cfg.unscatter[ch] = delta;
						scatter_fixed |= 1 << ch;
					}
				}
			}
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

// --------------------------------------------------------------------------
int TapeDrive::process(TapeChunk &chunk)
{
	int best_deskew;
	int least_errors;

	// load initial chunk config
	cfg = chunk.cfg;

	// try as is
	qDebug() << "initial try with deskew" << cfg.deskew;
	nrz1.process(chunk);

	if ((chunk.vpar_err_count == 0) && (chunk.crc_tape == chunk.crc_data) && (chunk.hpar_tape == chunk.hpar_data)) goto fin;

	best_deskew = cfg.deskew;
	least_errors = chunk.vpar_err_count;

	if (cfg.deskew_auto) {
		// wiggle deskew
		for (int deskew=cfg.bpl*0.2 ; deskew<=cfg.bpl*0.8 ; deskew++) {
			cfg.deskew = deskew;
			nrz1.process(chunk);
			qDebug() << "try with deskew" << cfg.deskew;
			if ((chunk.vpar_err_count == 0) && (chunk.crc_tape == chunk.crc_data) && (chunk.hpar_tape == chunk.hpar_data)) goto fin;
			if (chunk.vpar_err_count < least_errors) {
				least_errors = chunk.vpar_err_count;
				best_deskew = cfg.deskew;
			}
		}
		cfg.deskew = best_deskew;
	}

	if (cfg.unscatter_auto) {
		// unscatter again
		unscatter(chunk);
		nrz1.process(chunk);
		qDebug() << "after unscatter try with deskew" << cfg.deskew;
	}

	if (chunk.type != C_BLOCK) goto fin;

	best_deskew = cfg.deskew;
	least_errors = chunk.vpar_err_count;

	if (cfg.deskew_auto) {
		// wiggle deskew
		for (int deskew=cfg.bpl*0.2 ; deskew<=cfg.bpl*0.8 ; deskew++) {
			cfg.deskew = deskew;
			nrz1.process(chunk);
			qDebug() << "try with deskew" << cfg.deskew;
			if ((chunk.vpar_err_count == 0) && (chunk.crc_tape == chunk.crc_data) && (chunk.hpar_tape == chunk.hpar_data)) goto fin;
			if (chunk.vpar_err_count < least_errors) {
				least_errors = chunk.vpar_err_count;
				best_deskew = cfg.deskew;
			}
		}
		cfg.deskew = best_deskew;
	}

	if ((chunk.vpar_err_count == 0) && (chunk.crc_tape == chunk.crc_data) && (chunk.hpar_tape == chunk.hpar_data)) goto fin;

	if (cfg.unscatter_auto) {
		// wiggle unscatter
		wiggle_wiggle_wiggle(chunk);
		nrz1.process(chunk);
		qDebug() << "after wiggle try with deskew" << cfg.deskew;
	}
	best_deskew = cfg.deskew;
	least_errors = chunk.vpar_err_count;

	if (cfg.deskew_auto) {
		// wiggle deskew
		for (int deskew=cfg.bpl*0.2 ; deskew<=cfg.bpl*0.8 ; deskew++) {
			cfg.deskew = deskew;
			nrz1.process(chunk);
			qDebug() << "try with deskew" << cfg.deskew;
			if ((chunk.vpar_err_count == 0) && (chunk.crc_tape == chunk.crc_data) && (chunk.hpar_tape == chunk.hpar_data)) goto fin;
			if (chunk.vpar_err_count < least_errors) {
				least_errors = chunk.vpar_err_count;
				best_deskew = cfg.deskew;
			}
		}
		cfg.deskew = best_deskew;
	}

fin:
	// store configuration that chunk was finally processed with
	chunk.cfg = cfg;

	return VT_OK;
}
