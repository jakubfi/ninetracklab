#ifndef NINETRACKLAB_H
#define NINETRACKLAB_H

#include <QMainWindow>
#include <QModelIndex>

#include "tapedrive.h"
#include "histogramdialog.h"
#include "blockstore.h"

namespace Ui {
class ninetracklab;
}

// --------------------------------------------------------------------------
class NineTrackLab : public QMainWindow
{
private:
	Q_OBJECT
	Ui::ninetracklab *ui;
	HistogramDialog *hist;

	TDConf cfg;
	TapeDrive td;
	QMap<unsigned, TapeChunk> bs;
	QList<long> chunkstart;

	void updateUiFromConfig(TDConf &config);
	void updateChunkList();

private slots:
	void on_actionImport_triggered();
	void on_action_Save_View_As_triggered();
	void on_actionAbout_triggered();
	void on_actionPreprecessing_triggered();
	void on_actionSignal_histogram_triggered();
	void on_actionStart_analysis_triggered();
	void on_actionSlice_tape_triggered();
	void on_actionProcess_current_chunk_triggered();

	void on_edge_sens_currentIndexChanged(int edge_sens);
	void on_auto_unscatter_stateChanged(int arg);
	void on_deskew_valueChanged(int arg);
	void on_format_currentIndexChanged(int index);
	void on_bpl_valueChanged(int arg);
	void on_unscatter_changed(int arg);
	void on_chunks_activated(const QModelIndex &index);
	void on_deskew_auto_toggled(bool checked);

	void on_actionExport_blocks_triggered();

public:
	explicit NineTrackLab(QWidget *parent = 0);
	~NineTrackLab();

};

#endif

// vim: tabstop=4 shiftwidth=4 autoindent
