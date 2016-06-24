#include <QProgressDialog>
#include <QDebug>
#include <QTime>

#include "ninetracklab.h"
#include "tapedrive.h"

//static int defchmap[9] = { 7, 6, 5, 4, 3, 2, 1, 0, 8 };

// --------------------------------------------------------------------------
TapeDrive::TapeDrive(QWidget *parent)
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
			quint16 source_bit = (src_d >> cfg->chmap[ch]) & 1;
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

	int counter[9];
	qFill(counter, counter+9, cfg->realign_margin);
	int start_pos[9] = {0,0,0,0,0,0,0,0,0};
	int tail[9] = {0,0,0,0,0,0,0,0,0};

	for (int pos=100 ; pos<tape_samples-100 ; pos++) {
		for (int ch=0 ; ch<9 ; ch++) {
			int val = data[pos];
			int valp = data[pos-1];
			int row = val ^ valp;

			// fill the tail
			if (tail[ch] > 0) {
				data[pos] ^= 1 << ch;
				tail[ch]--;
			// found edge
			} else if ((row >> ch) & 1) {
				// second edge
				if (counter[ch] > 0) {
					for (int p=start_pos[ch] ; p<start_pos[ch]+cfg->realign_push ; p++) {
						data[p] ^= 1 << ch;
					}
					data[pos] ^= 1 << ch;
					tail[ch] = cfg->realign_push-1;
					counter[ch] = 0;
				// new edge
				} else {
					counter[ch] = cfg->realign_margin;
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
	qDebug() << "realign took" << ms << "ms";
}

// --------------------------------------------------------------------------
void TapeDrive::deglitch()
{
	QTime myTimer;
	myTimer.start();

	int start_pos[9] = {0,0,0,0,0,0,0,0,0};
	int maxlen = cfg->glitch_max;
	if ((maxlen < 1) && (cfg->glitch_single)) maxlen = 1;
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
							((cfg->glitch_single) && (pos-start_pos[ch] <= 1)) ||
							((pos-start_pos[ch] <= cfg->glitch_max) && (((data[pos-1]>>ch)&1) == ((data[pos+cfg->glitch_distance]>>ch)&1)))
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
int TapeDrive::preprocess()
{
	if (!tape) {
		return VT_ELOAD;
	}

	if (data) {
		delete[] data;
	}
	data = new quint16[tape_samples]();

	remap();
	deglitch();
	realign();

	qDebug() << "initial:" << getMissalign();
	return VT_OK;
}

// --------------------------------------------------------------------------
void TapeDrive::wiggle_wiggle_wiggle()
{
	QTime myTimer;
	myTimer.start();

	int scatter_initial = getMissalign();

	for (int ch=0 ; ch<9 ; ch++) {
		int iterations = 0;
		for (int dir=-1 ; dir <=1 ; dir+=2) {
			if (iterations <= 1) {
				forever {
					cfg->unscatter[ch] -= dir;
					iterations++;
					int cscatter = getMissalign();
					if (cscatter < scatter_initial) {
						qDebug() << ch << dir << " => (ok)" << cscatter;
						scatter_initial = cscatter;
					} else {
						qDebug() << ch << dir << " => (failure)" << cscatter;
						cfg->unscatter[ch] += dir;
						break;
					}
				}
			}
		}
	}

	int ms = myTimer.elapsed();
	qDebug() << "wiggle wiggle wiggle took" << ms << "ms";
}

// --------------------------------------------------------------------------
int TapeDrive::getMissalign(int edges_sample)
{
	int pulse_start;
	int last_pulse_start = 0;
	int bpulses = 0;

	int bpl = 25;
	int center = bpl * 0.4;
	int margin = 6;

	rewind();

	while (edges_sample > 0) {
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
void TapeDrive::unscatter()
{
	qDebug() << "unscatter";
	QTime myTimer;
	myTimer.start();

	int pulse_start = 0;
	int reference_start = 0;
	quint16 scatter_fixed;

	seek(0, TD_SEEK_END);
	scatter_fixed = 0;
	qFill(cfg->unscatter, cfg->unscatter+9, 0);

	int bpl = 25;
	int margin = bpl * 0.15;

	int pulse = read(&pulse_start, 0, EDGE_FALLING, DIR_BACKWARD);
	if (pulse < 0) {
		return;
	}
	for (int ch=0 ; ch<9 ; ch++) {
		if ((pulse>>ch)&1) {
			qDebug() << "track" << ch << "is initialy fixed";
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
						cfg->unscatter[ch] = delta;
						scatter_fixed |= 1 << ch;
						qDebug() << "fixed" << ch << "by" << delta;
					}
				}
			}
		}
	}
	qDebug() << "done after" << tape_samples-pos << "samples";
	qDebug() << "after unscatter:" << getMissalign();
	int ms = myTimer.elapsed();
	qDebug() << "unscatter took" << ms << "ms";
	wiggle_wiggle_wiggle();
	qDebug() << "after wiggle:" << getMissalign();
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
int TapeDrive::peek(int p)
{
	int pulse = 0;
	for (int ch=0 ; ch<9 ; ch++) {
		int uspos = p - cfg->unscatter[ch];
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
		int uspos = p - cfg->unscatter[ch];
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
