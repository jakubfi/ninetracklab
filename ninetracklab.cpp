#include <QFileDialog>
#include <QDialog>
#include <QDebug>

#include "ninetracklab.h"
#include "ui_ninetracklab.h"
#include "aboutdialog.h"
#include "preprocessdialog.h"
#include "histogramdialog.h"

/*
 * Debug levels:
 *  1 - top functions
 *  2 - pulse search failures
 *  3 - main segments notifications (mark, pulse)
 *  4 - inner-segment sections
 *  5 - inner-segment details
 *  6 -
 *  7 -
 *  8 -
 *  9 - deskewer vtape_get_pulse()
 */

static int debug_level;

// -----------------------------------------------------------------------
void DBG_ON(int level)
{
	debug_level = level;
}

// -----------------------------------------------------------------------
void DBG(int level, const char *format, ...)
{
	if (debug_level < level) return;

	va_list ap;
	qDebug() << QString().vsprintf(format, ap);

}

// --------------------------------------------------------------------------
NineTrackLab::NineTrackLab(QWidget *parent) : QMainWindow(parent), ui(new Ui::ninetracklab), td(this), bs(this)
{
	debug_level = 0;
	ui->setupUi(this);
	ui->tapeview->ConnectDataSources(&td, &bs);
	hist = new HistogramDialog(this, &td);
	setDeskew();
	on_edge_sens_currentIndexChanged(ui->edge_sens->currentIndex());
	setNRZ1();
	setPE();
}

// --------------------------------------------------------------------------
NineTrackLab::~NineTrackLab()
{
	delete ui;
	delete hist;
}

// --------------------------------------------------------------------------
void NineTrackLab::on_actionImport_triggered()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Import tape image"), "", tr("Binary files (*.bin);;All files (*)"));

	PreprocessDialog pp(this, &td);
	pp.exec();

	td.load(fileName);
	setWindowTitle(QString("Nine Track Lab - " + fileName.section("/",-1,-1)));
	td.preprocess();
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
	PreprocessDialog pp(this, &td);
	pp.exec();
}

// -----------------------------------------------------------------------
void NineTrackLab::on_actionSignal_histogram_triggered()
{
	if (hist->isHidden()) {
		hist->show();
	}
}

// -----------------------------------------------------------------------
void NineTrackLab::on_actionStart_analysis_triggered()
{
	bs.clear();

	switch (ui->format->currentIndex()) {
	case 0: // NRZ1
		nrz1.run(td, bs);
		break;
	case 1: // PE
		pe.run(td, bs);
		break;
	}
	ui->tapeview->update();
	ui->chunks->clear();
	int cnt = 0;
	QMap<unsigned, TapeChunk>::const_iterator i;
	for (i=bs.store.begin() ; i != bs.store.end() ; i++) {

		QString str = QString::number(i.value().bytes);
		str += " crc: " + QString::number(i.value().b_crc);
		str += "/" + QString::number(i.value().d_crc);
		str += " hpar: " + QString::number(i.value().b_hparity);
		str += "/" + QString::number(i.value().d_hparity);
		ui->chunks->addItem(str);

		QFile out(QString("blk") + QString::number(cnt));
		out.open(QIODevice::WriteOnly);
		out.write((const char*)i.value().data, i.value().bytes);
		out.close();
		cnt++;
	}
}

// -----------------------------------------------------------------------
void NineTrackLab::setDeskew()
{
	pe.set_deskew(ui->deskew->value());
	nrz1.set_deskew(ui->deskew->value());
}

// -----------------------------------------------------------------------
void NineTrackLab::setPE()
{
	pe.set_bpl(ui->bpl_min->value(), ui->bpl_max->value());
	pe.set_sync_pulses(ui->sync_pulses_min->value());
	pe.set_mark_pulses(ui->mark_pulses_min->value());
}

// -----------------------------------------------------------------------
void NineTrackLab::setNRZ1()
{
	nrz1.set_cksum_space(ui->cksum_min->value(), ui->cksum_max->value());
	nrz1.set_mark_space(ui->mark_min->value(), ui->mark_max->value());
}

// -----------------------------------------------------------------------
void NineTrackLab::on_edge_sens_currentIndexChanged(int edge_sens)
{
	pe.set_edge_sens(edge_sens+1);
	nrz1.set_edge_sens(edge_sens+1);
	ui->tapeview->setEdgeSens(edge_sens+1);
}

// -----------------------------------------------------------------------
void NineTrackLab::setScatter()
{
	QSpinBox *ch = (QSpinBox*) QObject::sender();
	int id = ch->objectName().replace("unscatter", "").toInt();
	td.set_scatter(id, ch->value());
	ui->tapeview->update();
}

// -----------------------------------------------------------------------
void NineTrackLab::updateScatter()
{
	for (int i=0 ; i<9 ; i++) {
		QSpinBox *ch = findChild<QSpinBox*>(QString("unscatter%1").arg(i));
		ch->setValue(td.get_scatter(i));
	}
}

// -----------------------------------------------------------------------
void NineTrackLab::on_actionUnscatter_triggered()
{
	td.unscatter();
	ui->tapeview->update();
	updateScatter();
}

// -----------------------------------------------------------------------
void NineTrackLab::on_auto_unscatter_stateChanged(int arg)
{
	for (int i=0 ; i<9 ; i++) {
		QSpinBox *ch = findChild<QSpinBox*>(QString("unscatter%1").arg(i));
		ch->setDisabled(arg);
	}
}
