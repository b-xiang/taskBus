#include "dlgabout.h"
#include "ui_dlgabout.h"
#include <QUrl>
DlgAbout::DlgAbout(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::DlgAbout)
{
	ui->setupUi(this);
	ui->textBrowser_About->
			setText("Taskbus is a cross-platform"
			 " multi-process cooperation framework for non-professional "
			 "developers, with four features of process based, "
			 "language independent, compiler independent, and architecture Independent.\n"
			 "by goldenhawking@163.com, Colored Eagle Studio, 2016,2017,2018,2019");
}

DlgAbout::~DlgAbout()
{
	delete ui;
}

void DlgAbout::on_pushButton_Next_clicked()
{
	this->accept();
}

void DlgAbout::on_pushButton_Close_clicked()
{
	this->reject();
}
