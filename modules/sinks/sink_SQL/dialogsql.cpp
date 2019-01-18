#include "dialogsql.h"
#include "ui_dialogsql.h"
#include "tb_interface.h"
#include <algorithm>
#include <QSqlDatabase>
DialogSQL::DialogSQL(const TASKBUS::cmdlineParser * ps, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::DialogSQL),
	m_pCmd(ps),
	m_rthread(new reciv_thread(ps,this)),
	m_pMod(new QStandardItemModel(this))
{
	ui->setupUi(this);
	connect(m_rthread,&reciv_thread::sig_quit,this,&DialogSQL::close);
	connect(m_rthread,&reciv_thread::new_message,this,&DialogSQL::deal_message,Qt::QueuedConnection);
	Qt::WindowFlags flg = windowFlags();
	flg |= Qt::WindowMinMaxButtonsHint;
	setWindowFlags(flg);
	m_rthread->start();
	ui->listView_SQL->setModel(m_pMod);
	QStringList lst = QSqlDatabase::drivers();
	foreach (QString l,lst)
		m_pMod->appendRow(new QStandardItem(l));
}

DialogSQL::~DialogSQL()
{
	m_rthread->terminate();
	m_rthread->wait();
	delete ui;
}

void DialogSQL::deal_message(QString package)
{
	m_pMod->appendRow(new QStandardItem(package));

	while (m_pMod->rowCount()>=256)
		m_pMod->removeRows(0,32);


}

