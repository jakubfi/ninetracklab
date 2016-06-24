#ifndef V9TTD_H
#define V9TTD_H

#include <QMainWindow>

#include "tapedrive.h"
#include "histogramdialog.h"
#include "blockstore.h"
#include "decodernrz1.h"
#include "decoderpe.h"

namespace Ui {
class ninetracklab;
}

// --------------------------------------------------------------------------
class NineTrackLab : public QMainWindow
{
private:
	Q_OBJECT
	Ui::ninetracklab *ui;
	TDConf cfg;
	HistogramDialog *hist;
	TapeDrive td;
	BlockStore bs;
	DecoderNRZ1 nrz1;
	DecoderPE pe;

	void updateUiFromConfig();

private slots:
	void setScatter();
	void on_actionImport_triggered();
	void on_actionAbout_triggered();
	void on_actionPreprecessing_triggered();
	void on_actionSignal_histogram_triggered();
	void on_actionStart_analysis_triggered();
	void on_edge_sens_currentIndexChanged(int edge_sens);
	void on_actionUnscatter_triggered();
	void on_auto_unscatter_stateChanged(int arg1);
	void on_deskew_valueChanged(int arg);

public:
	explicit NineTrackLab(QWidget *parent = 0);
	~NineTrackLab();

public slots:
	void setPE();
	void setNRZ1();
};

#endif // V9TTD_H
