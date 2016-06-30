#include <QDebug>

#include "preprocessdialog.h"
#include "ui_preprocessdialog.h"

// --------------------------------------------------------------------------
PreprocessDialog::PreprocessDialog(QWidget *parent, TDConf *c):
	QDialog(parent),
	ui(new Ui::PreprocessDialog)
{
	ui->setupUi(this);
	cfg = c;
	updateValues();
}

// --------------------------------------------------------------------------
void PreprocessDialog::updateValues()
{
	for (int i=0 ; i<9 ; i++) {
		QComboBox *ch = this->findChild<QComboBox*>(QString("track%1").arg(i));
		ch->setCurrentIndex(cfg->chmap[i]);
	}

	ui->format->setCurrentIndex(cfg->format == F_NRZ1 ? 0 : 1);
	ui->density_val->setValue(cfg->bpi);
	ui->tape_speed->setValue(cfg->tape_speed);
	ui->sampling_speed->setValue(cfg->sampling_speed);

	ui->realign_margin->setValue(cfg->realign_margin);
	ui->realign_push->setValue(cfg->realign_push);
	ui->glitch_max->setValue(cfg->glitch_max);
	ui->glitch_distance->setValue(cfg->glitch_distance);
	ui->glitch_single->setChecked(cfg->glitch_single);
}

// --------------------------------------------------------------------------
PreprocessDialog::~PreprocessDialog()
{
	delete ui;
}

// --------------------------------------------------------------------------
void PreprocessDialog::accept()
{
	close();
}

// --------------------------------------------------------------------------
void PreprocessDialog::on_format_currentIndexChanged(int index)
{
	switch (index) {
	case 0:
		cfg->setFormat(F_NRZ1);
		break;
	case 1:
		cfg->setFormat(F_PE);
		break;
	default:
		cfg->setFormat(F_NONE);
		break;
	}
	ui->bpl->setText(QString::number(cfg->bpl));
}

// --------------------------------------------------------------------------
void PreprocessDialog::on_sampling_speed_valueChanged(double arg)
{
	cfg->setSamplingSpeed(arg);
	ui->bpl->setText(QString::number(cfg->bpl));
}

// --------------------------------------------------------------------------
void PreprocessDialog::on_tape_speed_valueChanged(int arg)
{
	cfg->setTapeSpeed(arg);
	ui->bpl->setText(QString::number(cfg->bpl));
}

// --------------------------------------------------------------------------
void PreprocessDialog::on_density_select_currentIndexChanged(int index)
{
	if (index != 0) {
		int v = ui->density_select->currentText().toInt();
		ui->density_val->setValue(v);
		cfg->setBPI(v);
		ui->bpl->setText(QString::number(cfg->bpl));
	}
}

// --------------------------------------------------------------------------
void PreprocessDialog::on_density_val_valueChanged(int v)
{
	switch (v) {
	case 800:
		ui->density_select->setCurrentIndex(1);
		break;
	case 1600:
		ui->density_select->setCurrentIndex(2);
		break;
	case 3200:
		ui->density_select->setCurrentIndex(3);
		break;
	case 6250:
		ui->density_select->setCurrentIndex(4);
		break;
	default:
		ui->density_select->setCurrentIndex(0);
		break;
	}

	cfg->setBPI(v);
	ui->bpl->setText(QString::number(cfg->bpl));
}

// --------------------------------------------------------------------------
void PreprocessDialog::on_glitch_max_valueChanged(int arg)
{
	cfg->glitch_max = arg;
}

// --------------------------------------------------------------------------
void PreprocessDialog::on_glitch_distance_valueChanged(int arg)
{
	cfg->glitch_distance = arg;
}

// --------------------------------------------------------------------------
void PreprocessDialog::on_glitch_single_stateChanged(int arg)
{
	cfg->glitch_single = arg;
}

// --------------------------------------------------------------------------
void PreprocessDialog::on_realign_margin_valueChanged(int arg)
{
	cfg->realign_margin = arg;
}

// --------------------------------------------------------------------------
void PreprocessDialog::on_realign_push_valueChanged(int arg)
{
	cfg->realign_push = arg;
}

// vim: tabstop=4 shiftwidth=4 autoindent
