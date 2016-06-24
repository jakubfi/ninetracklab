#include <QFileDialog>
#include <QDialog>
#include <QDebug>

#include "ninetracklab.h"
#include "ui_ninetracklab.h"
#include "aboutdialog.h"
#include "preprocessdialog.h"
#include "histogramdialog.h"

// --------------------------------------------------------------------------
NineTrackLab::NineTrackLab(QWidget *parent) : QMainWindow(parent), ui(new Ui::ninetracklab), td(this), bs(this)
{
	ui->setupUi(this);
	ui->tapeview->useTapeDrive(&td);
	ui->tapeview->useBlockStore(&bs);
	ui->tapeview->useConfig(&cfg);
	hist = new HistogramDialog(this, &td);
	on_edge_sens_currentIndexChanged(ui->edge_sens->currentIndex());
	setNRZ1();
	setPE();
	td.useConfig(&cfg);
	updateUiFromConfig();
}

// --------------------------------------------------------------------------
NineTrackLab::~NineTrackLab()
{
	delete ui;
	delete hist;
}

// --------------------------------------------------------------------------
void NineTrackLab::updateUiFromConfig()
{
	// Input Signal
	ui->format->setCurrentIndex(cfg.format == F_NRZ1 ? 0 : 1);
	switch (cfg.edge_sens) {
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
	ui->bpl->setValue(cfg.bpl);

	// Scatter correction
	ui->unscatter_pull->setValue(1);
	ui->unscatter_est_width->setValue(1);
	for (int i=0 ; i<9 ; i++) {
		QSpinBox *ch = findChild<QSpinBox*>(QString("unscatter%1").arg(i));
		ch->setValue(cfg.unscatter[i]);
	}
	ui->deskew->setValue(cfg.deskew);

	// PE

	// NRZ1
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
		PreprocessDialog pp(this, &cfg);
		pp.exec();
		td.preprocess();
		updateUiFromConfig();
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
	td.preprocess();
	updateUiFromConfig();
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
void NineTrackLab::on_actionUnscatter_triggered()
{
	td.unscatter();
	ui->tapeview->update();
	updateUiFromConfig();
}

// -----------------------------------------------------------------------
// --- param changes -----------------------------------------------------
// -----------------------------------------------------------------------

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

	pe.set_edge_sens(edge_sens+1);
	nrz1.set_edge_sens(edge_sens+1);

}

// -----------------------------------------------------------------------
void NineTrackLab::setScatter()
{
	QSpinBox *ch = (QSpinBox*) QObject::sender();
	int id = ch->objectName().replace("unscatter", "").toInt();
	cfg.unscatter[id] = ch->value();
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
	pe.set_deskew(arg);
	nrz1.set_deskew(arg);
}

