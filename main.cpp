#include "ninetracklab.h"
#include <QApplication>

// --------------------------------------------------------------------------
int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	NineTrackLab w;
	w.show();

	return a.exec();
}

// vim: tabstop=4 shiftwidth=4 autoindent
