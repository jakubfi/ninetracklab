#ifndef TAPEDRIVE_H
#define TAPEDRIVE_H

#include <QFile>
#include <QString>
#include <QWidget>

#include "tdconf.h"

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
private:
	QWidget *parent;

	QFile tape_file;
	quint16 *tape;
	int tape_samples;
	quint16 *data;
	int pos;

	TDConf *cfg;

public:
	TapeDrive(QWidget *parent);
	~TapeDrive() { unload(); }

	void useConfig(TDConf *c) { cfg = c; }

	int parity9(quint8 x);
	int load(QString image);
	int unload();

	int preprocess();
	void remap();
	void realign();
	void deglitch();

	void unscatter();
	int getMissalign(int edges_sample = 100000);
	void wiggle_wiggle_wiggle();

	int tape_len() { return tape_samples; }
	quint16 * tape_data() { return data; }
	int peek(int p);
	int get_edge_internal(int p, int edge, int dir);
	int get_edge(int p, int edge, int dir);
	int read(int *pulse_start, int deskew_max, int edge, int dir = DIR_FORWARD);
	void rewind() { seek(0, TD_SEEK_SET); }
	int seek(unsigned long offset, int whence);
	int get_pos() { return pos; }
	int tape_loaded() { return (data ? 1 : 0); }

};

#endif // TAPEDRIVE_H
