#include "mainwindow.h"
#include <QApplication>

// --------------------------------------------------------------------------
int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	v9ttd w;
	w.show();

	return a.exec();
}
