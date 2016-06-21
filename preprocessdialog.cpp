#include "preprocessdialog.h"
#include "ui_preprocessdialog.h"
#include "tapedrive.h"

// --------------------------------------------------------------------------
PreprocessDialog::PreprocessDialog(QWidget *parent, TapeDrive *td) :
	QDialog(parent),
	ui(new Ui::PreprocessDialog)
{
	ui->setupUi(this);
	this->td = td;
	fcpi = 0;

	for (int i=0 ; i<9 ; i++) {
		QComboBox *ch = this->findChild<QComboBox*>(QString("track%1").arg(i));
		ch->setCurrentIndex(td->get_chnum(i));
	}

	ui->realign_margin->setValue(td->get_realign_margin());
	ui->realign_push->setValue(td->get_realign_push());
	ui->glitch_max->setValue(td->get_glitch_max());
	ui->glitch_distance->setValue(td->get_glitch_distance());
	ui->glitch_single->setChecked(td->get_glitch_single());
}

// --------------------------------------------------------------------------
PreprocessDialog::~PreprocessDialog()
{
	delete ui;
}

// --------------------------------------------------------------------------
void PreprocessDialog::accept()
{
	for (int i=0 ; i<9 ; i++) {
		QComboBox *ch = this->findChild<QComboBox*>(QString("track%1").arg(i));
		td->set_chmap(i, ch->currentIndex());
	}

	td->set_realign(ui->realign_margin->value(), ui->realign_push->value());
	td->set_glitch(ui->glitch_max->value(), ui->glitch_distance->value(), ui->glitch_single->isChecked());

	td->preprocess();

	close();
}

// --------------------------------------------------------------------------
void PreprocessDialog::updateBPL()
{
	double spfc = (1000000 * ui->sampling_speed->value()) / (fcpi * ui->tape_speed->value());
	ui->bpl->setValue(qRound(spfc));
}

// --------------------------------------------------------------------------
void PreprocessDialog::on_tape_type_currentIndexChanged(int index)
{
	if (index == 0) {
		ui->tape_speed->setEnabled(false);
		ui->sampling_speed->setEnabled(false);
		ui->bpl->setEnabled(true);
		fcpi = 0;
	} else {
		ui->tape_speed->setEnabled(true);
		ui->sampling_speed->setEnabled(true);
		ui->bpl->setEnabled(false);
		switch (index) {
		case 1:
			fcpi = 800;
			break;
		case 2:
			fcpi = 3200;
			break;
		}
		updateBPL();
	}
}

// --------------------------------------------------------------------------
void PreprocessDialog::on_sampling_speed_valueChanged(double arg)
{
	updateBPL();
}

// --------------------------------------------------------------------------
void PreprocessDialog::on_tape_speed_valueChanged(int arg)
{
	updateBPL();
}
