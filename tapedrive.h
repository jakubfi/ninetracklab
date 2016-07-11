#ifndef TAPEDRIVE_H
#define TAPEDRIVE_H

#include <QFile>
#include <QString>
#include <QWidget>

#include "tdconf.h"
#include "decodernrz1.h"
#include "decoderpe.h"

enum td_error_codes {
	VT_OK = 0,
	VT_EOT = -1,
	VT_EPULSE = -2,
	VT_ELOAD = -3,
};

enum tape_direction { DIR_FORWARD = 1, DIR_BACKWARD = -1 };
enum tape_seek_whence { TD_SEEK_SET, TD_SEEK_CUR, TD_SEEK_END };


// --------------------------------------------------------------------------
class TapeDrive : QObject
{
	friend class TapeView;
	friend class DecoderNRZ1;
	friend class DecoderPE;

private:
	QWidget *parent;

	QFile tape_file;
	quint16 *tape;
	int tape_samples;
	quint16 *data;
	int pos;

	TDConf cfg;
	DecoderNRZ1 nrz1;
	DecoderPE pe;

private:
	void remap();
	void realign();
	void deglitch_old();
	void deglitch_new();
	int getMissalign(TapeChunk &chunk, int edges_sample = 10000000);
	int get_edge_internal(int p, int edge, int dir);
	int get_edge(int p, int edge, int dir);
	void unscatter(TapeChunk &chunk);
	void wiggle_wiggle_wiggle(TapeChunk &chunk);

public:
	TapeDrive(QWidget *parent);
	~TapeDrive() { unload(); }

	int parity9(quint8 x);
	int load(QString image);
	int unload();
	void exportCut(QString filename, long left, long right);

	int preprocess(TDConf &c);

	int tape_len() { return tape_samples; }
	int tape_loaded() { return (data ? 1 : 0); }
	quint16 * tape_data() { return data; }

	int peek(int p);
	int read(int *pulse_start, int deskew_max, int edge, int dir = DIR_FORWARD);
	void rewind() { seek(0, TD_SEEK_SET); }
	int seek(unsigned long offset, int whence);
	int get_pos() { return pos; }

	TapeChunk scan_next_chunk(int start);
	void bitFix(TapeChunk &chunk);
	int process(TapeChunk &chunk);

};

#endif

// vim: tabstop=4 shiftwidth=4 autoindent
