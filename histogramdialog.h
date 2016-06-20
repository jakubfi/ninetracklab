#ifndef HISTOGRAMDIALOG_H
#define HISTOGRAMDIALOG_H

#include <QDialog>

#include "tapedrive.h"

namespace Ui {
class HistogramDialog;
}

// --------------------------------------------------------------------------
class HistogramDialog : public QDialog
{
private:
	Q_OBJECT
	Ui::HistogramDialog *ui;
	TapeDrive *td;

public:
	explicit HistogramDialog(QWidget *parent, TapeDrive *td);
	~HistogramDialog();

public slots:
	void hupdate();
};

#endif // HISTOGRAMDIALOG_H
