#include "aboutdialog.h"
#include "ui_aboutdialog.h"

// --------------------------------------------------------------------------
AboutDialog::AboutDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::AboutDialog)
{
	ui->setupUi(this);
}

// --------------------------------------------------------------------------
AboutDialog::~AboutDialog()
{
	delete ui;
}

// vim: tabstop=4 shiftwidth=4 autoindent
