#include "formstatus.h"
#include "ui_formstatus.h"
#include <QDateTime>
FormStatus::FormStatus(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::FormStatus),
	m_watchModule(new WatchMemModule(this))
{
	ui->setupUi(this);
	m_nTmid = startTimer(1000);
	//Mem watch
	ui->tableView_memstatus->setModel(m_watchModule);
}

FormStatus::~FormStatus()
{
	delete ui;

}

void FormStatus::timerEvent(QTimerEvent *event)
{
	if (m_nTmid==event->timerId())
		update_charts();
}
void FormStatus::update_charts()
{
	if (m_watchModule)
	{
		m_watchModule->update_items();
	}
}
