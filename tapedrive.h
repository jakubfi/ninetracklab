#ifndef TAPEDRIVE_H
#define TAPEDRIVE_H

#include <QFile>
#include <QString>
#include <QWidget>

enum td_error_codes {
	VT_OK = 0,
	VT_EOT = -1,
	VT_EPULSE = -2,
	VT_ELOAD = -3,
};

enum tape_formats { F_NONE, F_PE, F_NRZ1 };
enum edge_dir { EDGE_NONE, EDGE_RISING, EDGE_FALLING, EDGE_ANY };
enum tape_seek_whence { TD_SEEK_SET, TD_SEEK_CUR, TD_SEEK_END };

// --------------------------------------------------------------------------
class TapeDrive : QObject
{
private:
	QWidget *parent;

	// tape
	QFile tape_file;
	quint16 *tape;
	int tape_samples;
	quint16 *data;
	int pos;

	// preprocessing
	int chmap[9];
	int scatter[9];
	quint16 scatter_fixed;
	int realign_margin;
	int realign_push;
	int glitch_max;
	int glitch_distance;
	bool glitch_single;

public:
	TapeDrive(QWidget *parent);
	~TapeDrive() { unload(); }

	int parity9(quint8 x);
	int load(QString image);
	int unload();

	int preprocess();
	void remap();
	void realign();
	void deglitch();

	void unscatter();
	int getMissalign();
	void wiggle_wiggle_wiggle();

	void set_realign(int margin, int push) { realign_margin = margin; realign_push = push; }
	void set_scatter(int track, int sctr) { scatter[track] = sctr; }
	void set_glitch(int glt, int dst, bool single) { glitch_max = glt; glitch_distance = dst; glitch_single = single; }
	void set_chmap(int track, int channel) { chmap[track] = channel; }

	int get_realign_margin() { return realign_margin; }
	int get_realign_push() { return realign_push; }
	int get_scatter(int track) { return scatter[track]; }
	int get_glitch_max() { return glitch_max; }
	int get_glitch_distance() { return glitch_distance; }
	bool get_glitch_single() { return glitch_single; }
	int get_chnum(int track) { return chmap[track]; }

	int tape_len() { return tape_samples; }
	quint16 * tape_data() { return data; }
	int peek(int p);
	int get_edge_internal(int p, int edge, int *rising = NULL);
	int get_edge(int p, int edge, int *rising = NULL);
	int read(int *pulse_start, int deskew_max, int edge);
	void rewind() { seek(0, TD_SEEK_SET); }
	int seek(unsigned long offset, int whence);
	int get_pos() { return pos; }
	int is_loaded() { return (tape ? 1 : 0); }

};

#endif // TAPEDRIVE_H
