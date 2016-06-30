#ifndef PREPROCESSDIALOG_H
#define PREPROCESSDIALOG_H

#include "tdconf.h"

#include <QDialog>

namespace Ui {
class PreprocessDialog;
}

// --------------------------------------------------------------------------
class PreprocessDialog : public QDialog
{
private:
	Q_OBJECT
	Ui::PreprocessDialog *ui;
	TDConf *cfg;

	void updateValues();

public:
	explicit PreprocessDialog(QWidget *parent, TDConf *c);
	~PreprocessDialog();

public slots:
	void accept();

private slots:
	void on_sampling_speed_valueChanged(double arg);
	void on_tape_speed_valueChanged(int arg);
	void on_density_select_currentIndexChanged(int index);
	void on_density_val_valueChanged(int v);
	void on_format_currentIndexChanged(int index);
	void on_glitch_max_valueChanged(int arg);
	void on_glitch_distance_valueChanged(int arg);
	void on_glitch_single_stateChanged(int arg);
	void on_realign_margin_valueChanged(int arg);
	void on_realign_push_valueChanged(int arg);
};

#endif

// vim: tabstop=4 shiftwidth=4 autoindent
