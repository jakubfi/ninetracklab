#ifndef PREPROCESSDIALOG_H
#define PREPROCESSDIALOG_H

#include <QDialog>

#include "tapedrive.h"

namespace Ui {
class PreprocessDialog;
}

class PreprocessDialog : public QDialog
{
private:
	Q_OBJECT
	Ui::PreprocessDialog *ui;
	TapeDrive *td;
	long fcpi;

	void updateBPL();

public:
	explicit PreprocessDialog(QWidget *parent, TapeDrive *td);
	~PreprocessDialog();

public slots:
	void accept();
private slots:
	void on_tape_type_currentIndexChanged(int index);
	void on_sampling_speed_valueChanged(double arg1);
	void on_tape_speed_valueChanged(int arg1);
};

#endif // PREPROCESSDIALOG_H
