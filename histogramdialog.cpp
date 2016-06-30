#include <QProgressDialog>

#include "histogramdialog.h"
#include "ui_histogramdialog.h"
#include "tapedrive.h"

// --------------------------------------------------------------------------
HistogramDialog::HistogramDialog(QWidget *parent, TapeDrive *td) : QDialog(parent), ui(new Ui::HistogramDialog)
{
	ui->setupUi(this);
	this->td = td;
}

// --------------------------------------------------------------------------
HistogramDialog::~HistogramDialog()
{
	delete ui;
}

// --------------------------------------------------------------------------
void HistogramDialog::hupdate()
{
	int pulse;
	//int last_change[9] = { 0,0,0,0,0,0,0,0,0 };
	int pulse_start;
	int last_start = 0;

	td->rewind();
	ui->histogram->setup(ui->min->value(), ui->max->value(), ui->step->value());

	int edge_sens = ui->rising->isChecked() ? EDGE_RISING : 0;
	edge_sens |= ui->falling->isChecked() ? EDGE_FALLING : 0;

	QProgressDialog progress("Updating histogram...", "Abort", 0, 100, this);
	progress.setWindowTitle("Histogram update");
	progress.setWindowModality(Qt::WindowModal);
	progress.show();

	int len = td->tape_len();
	int mod = len/100;

	while ((pulse = td->read(&pulse_start, 0, edge_sens)) != VT_EOT) {
		int spos = td->get_pos();
		if (spos % mod == 0) {
			progress.setValue(spos/len);
		}
		ui->histogram->inc(pulse_start-last_start);
		last_start = pulse_start;
		/*
		for (int c=0 ; c<9 ; c++) {
			if (pulse & (1<<c)) {
				int len = spos - last_change[c];
				ui->histogram->inc(len);
				last_change[c] = spos;
			}
		}
		*/
	}

	ui->mfp->setText(QString("Most Frequent Pulse: %1").arg(ui->histogram->get_mfp()));
	ui->histogram->update();

}

// vim: tabstop=4 shiftwidth=4 autoindent
