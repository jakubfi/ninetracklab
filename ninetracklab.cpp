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
	updateUiFromConfig(cfg);
}

// --------------------------------------------------------------------------
NineTrackLab::~NineTrackLab()
{
	delete ui;
	delete hist;
}

// --------------------------------------------------------------------------
void NineTrackLab::updateUiFromConfig(TDConf &config)
{
	// Input Signal
	ui->format->setCurrentIndex(config.format == F_NRZ1 ? 0 : 1);
	switch (config.edge_sens) {
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
	ui->bpl->setValue(config.bpl);

	// Scatter correction
	ui->auto_unscatter->setChecked(config.unscatter_auto);
	ui->unscatter_pull->setValue(1);
	ui->unscatter_est_width->setValue(1);
	for (int i=0 ; i<9 ; i++) {
		QSpinBox *ch = findChild<QSpinBox*>(QString("unscatter%1").arg(i));
		ch->setValue(config.unscatter[i]);
	}

	// Deskew
	ui->deskew->setValue(config.deskew);
	ui->deskew_auto->setChecked(config.deskew_auto);

	// PE

	// NRZ1
}

// -----------------------------------------------------------------------
void NineTrackLab::updateChunkList()
{
	int current_idx = -1;
	if (ui->chunks->currentIndex().isValid()) {
		current_idx = ui->chunks->currentIndex().row();
	}

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

		QColor color;
		switch (i.value().type) {
		case C_BLOCK:
			blocks++;
			str = QString("%1 block, %2 bytes").arg(format).arg(i.value().bytes);
			break;
		case C_MARK:
			color = QColor(220, 255, 220);
			marks++;
			str = QString("%1 tape mark").arg(format);
			break;
		default:
			color = QColor(Qt::white);
			str = QString("chunk @%1, %2 smp").arg(i.value().beg).arg(i.value().len);
			break;
		}
		if (i.value().type == C_BLOCK) {
			if ((i.value().crc_tape == i.value().crc_data) && (i.value().hpar_tape == i.value().hpar_data)) {
				color = QColor(220, 255, 220);
			} else if (i.value().fixed) {
				color = QColor(255, 255, 200);
			} else {
				errors++;
				color = QColor(255, 220, 220);
				str += "[";
				if (i.value().crc_tape != i.value().crc_data) str += "C";
				if (i.value().hpar_err) str += "H";
				if (i.value().vpar_err_count != 0) str += "V";
				str += "]";
			}
		}
		QListWidgetItem *item = new QListWidgetItem(str);
		item->setBackgroundColor(color);
		ui->chunks->addItem(item);
		chunkstart.append(i.value().beg);
	}
	ui->chunks->setCurrentRow(current_idx);
	QString status = QString("%1 chunks, %2 blocks, %3 marks, %4 errors, %5 unknown").arg(chunks).arg(blocks).arg(marks).arg(errors).arg(chunks-blocks-marks);
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
		cfg = TDConf();
		td.load(fileName);
		setWindowTitle(QString("Nine Track Lab - " + fileName.section("/",-1,-1)));
		PreprocessDialog pp(this, &cfg);
		pp.exec();
		updateUiFromConfig(cfg);
		td.preprocess(cfg);
	}
}

// -----------------------------------------------------------------------
void NineTrackLab::on_action_Save_View_As_triggered()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save tape image cut"), "", tr("Binary files (*.bin);;All files (*)"));
	if (!fileName.isEmpty()) {
		td.exportCut(fileName, ui->tapeview->leftSample(), ui->tapeview->rightSample());
	}
}

// -----------------------------------------------------------------------
void NineTrackLab::on_actionExport_blocks_triggered()
{
	int cnt = 0;
	char buf[16*1024];
	QMap<unsigned, TapeChunk>::iterator i;
	for (i=bs.begin() ; i!=bs.end() ; i++) {
		if (i.value().type == C_NONE) continue;
		QString filename = QString("blk_%1_%2").arg(cnt, 4, 10, QChar('0')).arg(i.value().beg);
		if (i.value().type == C_MARK) {
			filename += ".mrk";
		} else if (i.value().type == C_BLOCK) {
			if (i.value().crc_data == i.value().crc_tape) {
				filename += ".blk";
			} else {
				filename += "_crc.blk";
			}
		} else {
			filename += "___";
		}
		for (int p=0 ; p<i.value().bytes ; p++) {
			buf[p] = i.value().data[p];
		}
		QFile out(filename);
		out.open(QIODevice::WriteOnly);
		out.write((const char*)buf, i.value().bytes);
		out.close();
		cnt++;
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
	PreprocessDialog pp(this, &cfg);
	pp.exec();
	updateUiFromConfig(cfg);
	td.preprocess(cfg);
}

// -----------------------------------------------------------------------
void NineTrackLab::on_actionSignal_histogram_triggered()
{
	if (hist->isHidden()) {
		hist->show();
	}
}

// -----------------------------------------------------------------------
void NineTrackLab::on_actionSlice_tape_triggered()
{
	bs.clear();
	int start = 0;
	forever {
		TapeChunk chunk = td.scan_next_chunk(start);
		chunk.cfg = cfg;
		if (chunk.end >= 0) {
			bs.insert(chunk.beg, chunk);
		} else {
			break;
		}
		start = chunk.end;
	}
	updateChunkList();
	ui->tapeview->update();
}

// -----------------------------------------------------------------------
void NineTrackLab::on_actionStart_analysis_triggered()
{
	QTime myTimer;
	myTimer.start();

	QMap<unsigned, TapeChunk>::iterator i;
	for (i=bs.begin() ; i!=bs.end() ; i++) {
		i.value().cfg = cfg;
		td.process(i.value());
		cfg = i.value().cfg;
	}
	updateChunkList();
	ui->tapeview->update();

	qDebug() << "total processing time:" << myTimer.elapsed();
}

// -----------------------------------------------------------------------
void NineTrackLab::on_actionProcess_current_chunk_triggered()
{
	QModelIndex idx = ui->chunks->currentIndex();
	if (idx.isValid()) {
		TapeChunk &chunk = bs[chunkstart[idx.row()]];
		chunk.cfg = cfg;
		td.process(chunk);
		cfg = chunk.cfg;
		updateUiFromConfig(cfg);
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
	updateUiFromConfig(chunk.cfg);
}

// -----------------------------------------------------------------------
// --- param changes -----------------------------------------------------
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
void NineTrackLab::on_edge_sens_currentIndexChanged(int edge_sens)
{
	switch (edge_sens) {
	case 0:
		cfg.edge_sens = EDGE_RISING;
		break;
	case 1:
		cfg.edge_sens = EDGE_FALLING;
		break;
	default:
		cfg.edge_sens = EDGE_ANY;
		break;
	}
	ui->tapeview->update();
}

// -----------------------------------------------------------------------
void NineTrackLab::on_unscatter_changed(int arg)
{
	QSpinBox *ch = (QSpinBox*) QObject::sender();
	int id = ch->objectName().replace("unscatter", "").toInt();
	cfg.unscatter[id] = arg;
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
	cfg.unscatter_auto = arg;
}

// -----------------------------------------------------------------------
void NineTrackLab::on_deskew_valueChanged(int arg)
{
	cfg.deskew = arg;
}

// -----------------------------------------------------------------------
void NineTrackLab::on_format_currentIndexChanged(int index)
{
	switch (index) {
	case 0:
		cfg.setFormat(F_NRZ1);
		break;
	case 1:
		cfg.setFormat(F_PE);
		break;
	}
}

// -----------------------------------------------------------------------
void NineTrackLab::on_bpl_valueChanged(int arg)
{
	cfg.setBPL(arg);
}

// -----------------------------------------------------------------------
void NineTrackLab::on_deskew_auto_toggled(bool checked)
{
	cfg.deskew_auto = checked;
	ui->deskew->setDisabled(checked);
}

// vim: tabstop=4 shiftwidth=4 autoindent


