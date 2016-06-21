#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>

namespace Ui {
class AboutDialog;
}

// --------------------------------------------------------------------------
class AboutDialog : public QDialog
{
private:
	Q_OBJECT
	Ui::AboutDialog *ui;

public:
	explicit AboutDialog(QWidget *parent = 0);
	~AboutDialog();

};

#endif // ABOUTDIALOG_H
