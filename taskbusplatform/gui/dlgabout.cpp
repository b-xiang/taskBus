#include "dlgabout.h"
#include "ui_dlgabout.h"
#include <QUrl>
DlgAbout::DlgAbout(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::DlgAbout)
{
	ui->setupUi(this);
}

DlgAbout::~DlgAbout()
{
	delete ui;
}

void DlgAbout::on_pushButton_mainPage_clicked()
{
	QStringList ls;
	ls<<"https://github.com/";
	ui->textBrowser_About->setSource(QUrl::fromStringList(ls).first());
	ui->label->hide();
}

void DlgAbout::on_pushButton_documents_clicked()
{
	QStringList ls;
	ls<<"https://github.com/";
	ui->textBrowser_About->setSource(QUrl::fromStringList(ls).first());
	ui->label->hide();
}

void DlgAbout::on_pushButton_Next_clicked()
{
	this->accept();
}

void DlgAbout::on_pushButton_Close_clicked()
{
	this->reject();
}
