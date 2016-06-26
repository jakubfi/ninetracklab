#include <QFileDialog>
#include <QDialog>
#include <QDebug>
#include <QTime>

#include "ninetracklab.h"
#include "ui_ninetracklab.h"
#include "aboutdialog.h"
#include "preprocessdialog.h"
#include "histogramdialog.h"

// --------------------------------------------------------------------------
NineTrackLab::NineTrackLab(QWidget *parent) : QMainWindow(parent), ui(new Ui::ninetracklab), td(this)
{
	ui->setupUi(this);
	ui->tapeview->useTapeDrive(&td);
	ui->tapeview->useBlockStore(&bs);
	hist = new HistogramDialog(this, &td);
	this->cfg = td.getConfig();
	updateUiFromConfig(cfg);
}

// --------------------------------------------------------------------------
NineTrackLab::~NineTrackLab()
{
	delete ui;
	delete hist;
}

// --------------------------------------------------------------------------
void NineTrackLab::updateUiFromConfig(TDConf *config)
{
	// Input Signal
	ui->format->setCurrentIndex(config->format == F_NRZ1 ? 0 : 1);
	switch (config->edge_sens) {
	case EDGE_ANY:
	case EDGE_NONE:
		ui->edge_sens->setCurrentIndex(2);
		break;
	case EDGE_FALLING:
		ui->edge_sens->setCurrentIndex(1);
		break;
	case EDGE_RISING:
		ui->edge_sens->setCurrentIndex(0);
		break;
	}
	ui->bpl->setValue(config->bpl);

	// Scatter correction
	ui->unscatter_pull->setValue(1);
	ui->unscatter_est_width->setValue(1);
	for (int i=0 ; i<9 ; i++) {
		QSpinBox *ch = findChild<QSpinBox*>(QString("unscatter%1").arg(i));
		ch->setValue(config->unscatter[i]);
	}
	ui->deskew->setValue(config->deskew);

	// PE

	// NRZ1
}

// -----------------------------------------------------------------------
void NineTrackLab::updateChunkList()
{
//	int cnt = 0;
	ui->chunks->clear();
	chunkstart.clear();
	QMap<unsigned, TapeChunk>::const_iterator i;
	QString str;
	QString format;
	int chunks = 0;
	int blocks = 0;
	int marks = 0;
	int errors = 0;

	for (i=bs.begin() ; i!=bs.end() ; i++) {
		chunks++;
		switch (i.value().format) {
		case F_NRZ1:
			format = "NRZ1";
			break;
		case F_PE:
			format = "PE";
			break;
		default:
			format = "???";
			break;
		}

		QColor color = QColor(Qt::white);
		switch (i.value().type) {
		case C_BLOCK:
			blocks++;
			str = QString("%1 block, %2 bytes").arg(format).arg(i.value().bytes);
			break;
		case C_MARK:
			marks++;
			str = QString("%1 tape mark").arg(format);
			break;
		default:
			str = QString("chunk @%1, %2 smp").arg(i.value().beg).arg(i.value().len);
			break;
		}
		if (i.value().format != F_NONE) {
			if ((i.value().b_crc == i.value().d_crc) && (i.value().b_hparity == i.value().d_hparity)) {
				color = QColor(220, 255, 220);
			} else {
				errors++;
				color = QColor(255, 220, 220);
				str += "[";
				if (i.value().b_crc != i.value().d_crc) str += "C";
				if (i.value().b_hparity == i.value().d_hparity) str += "P";
				str += "]";
			}
		}
		QListWidgetItem *item = new QListWidgetItem(str);
		item->setBackgroundColor(color);
		ui->chunks->addItem(item);
		chunkstart.append(i.value().beg);
/*
		QFile out(QString("blk") + QString::number(cnt));
		out.open(QIODevice::WriteOnly);
		out.write((const char*)i.value().data, i.value().bytes);
		out.close();
		cnt++;
*/
	}
	QString status = QString("%1 chunks, %2 blocks, %3 marks, %4 errors").arg(chunks).arg(blocks).arg(marks).arg(errors);
	ui->statusBar->showMessage(status);
}

// --------------------------------------------------------------------------
// --- actions --------------------------------------------------------------
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
void NineTrackLab::on_actionImport_triggered()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Import tape image"), "", tr("Binary files (*.bin);;All files (*)"));

	if (!fileName.isEmpty()) {
		td.load(fileName);
		setWindowTitle(QString("Nine Track Lab - " + fileName.section("/",-1,-1)));
		PreprocessDialog pp(this, cfg);
		pp.exec();
		td.preprocess();
	}
}

// -----------------------------------------------------------------------
void NineTrackLab::on_action_Save_View_As_triggered()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Save tape image cut"), "", tr("Binary files (*.bin);;All files (*)"));
	if (!fileName.isEmpty()) {
		td.exportCut(fileName, ui->tapeview->leftSample(), ui->tapeview->rightSample());
	}
}

// -----------------------------------------------------------------------
void NineTrackLab::on_actionAbout_triggered()
{
	AboutDialog about(this);
	about.exec();
}

// -----------------------------------------------------------------------
void NineTrackLab::on_actionPreprecessing_triggered()
{
	PreprocessDialog pp(this, cfg);
	pp.exec();
	td.preprocess();
}

// -----------------------------------------------------------------------
void NineTrackLab::on_actionSignal_histogram_triggered()
{
	if (hist->isHidden()) {
		hist->show();
	}
}

// -----------------------------------------------------------------------
void NineTrackLab::on_actionUnscatter_triggered()
{

}

// -----------------------------------------------------------------------
void NineTrackLab::on_actionSlice_tape_triggered()
{
	bs.clear();
	int start = 0;
	forever {
		TapeChunk chunk = td.scan_next_chunk(start);
		start = chunk.end;
		if (start >= 0) {
			bs.insert(chunk.beg, chunk);
		} else {
			break;
		}
	}
	updateChunkList();
	ui->tapeview->update();
}

// -----------------------------------------------------------------------
void NineTrackLab::on_actionStart_analysis_triggered()
{
	QTime myTimer;
	myTimer.start();

	td.unscatter(bs.begin().value());
	td.wiggle_wiggle_wiggle(bs.begin().value());
	QMap<unsigned, TapeChunk>::iterator i;
	int cnt = bs.size();
	int c=0;
	for (i=bs.begin() ; i!=bs.end() ; i++) {
		qDebug() << c++ << "of" << cnt;
		td.process_auto(i.value());
	}
	updateChunkList();
	ui->tapeview->update();

	int ms = myTimer.elapsed();
	qDebug() << "whole analysis took " << ms << "ms";
}

// -----------------------------------------------------------------------
void NineTrackLab::on_actionProcess_current_chunk_triggered()
{
	QModelIndex idx = ui->chunks->currentIndex();
	if (idx.isValid()) {
		TapeChunk &chunk = bs[chunkstart[idx.row()]];
		td.process(chunk);
		updateChunkList();
		ui->tapeview->update();
	}
}

// -----------------------------------------------------------------------
void NineTrackLab::on_chunks_activated(const QModelIndex &index)
{
	int idx = index.row();
	TapeChunk &chunk = bs[chunkstart[idx]];
	ui->tapeview->zoomRegion(chunk.beg, chunk.end);
	updateUiFromConfig(&chunk.cfg);
}

// -----------------------------------------------------------------------
// --- param changes -----------------------------------------------------
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
void NineTrackLab::on_edge_sens_currentIndexChanged(int edge_sens)
{
	switch (edge_sens) {
	case 0:
		cfg->edge_sens = EDGE_RISING;
		break;
	case 1:
		cfg->edge_sens = EDGE_FALLING;
		break;
	default:
		cfg->edge_sens = EDGE_ANY;
		break;
	}
	ui->tapeview->update();
}

// -----------------------------------------------------------------------
void NineTrackLab::on_unscatter_changed(int arg)
{
	QSpinBox *ch = (QSpinBox*) QObject::sender();
	int id = ch->objectName().replace("unscatter", "").toInt();
	cfg->unscatter[id] = arg;
	ui->tapeview->update();
}

// -----------------------------------------------------------------------
void NineTrackLab::on_auto_unscatter_stateChanged(int arg)
{
	for (int i=0 ; i<9 ; i++) {
		QSpinBox *ch = findChild<QSpinBox*>(QString("unscatter%1").arg(i));
		ch->setDisabled(arg);
	}
	ui->unscatter_pull->setEnabled(arg);
	ui->unscatter_est_width->setEnabled(arg);
	cfg->unscatter_auto = arg;
}

// -----------------------------------------------------------------------
void NineTrackLab::on_deskew_valueChanged(int arg)
{
	cfg->deskew = arg;
}

// -----------------------------------------------------------------------
void NineTrackLab::on_format_currentIndexChanged(int index)
{
	switch (index) {
	case 0:
		cfg->setFormat(F_NRZ1);
		break;
	case 1:
		cfg->setFormat(F_PE);
		break;
	}
}

// -----------------------------------------------------------------------
void NineTrackLab::on_bpl_valueChanged(int arg)
{
	cfg->setBPL(arg);
}

