#include "formstatus.h"
#include "ui_formstatus.h"
#include <QDateTime>
FormStatus::FormStatus(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::FormStatus),
	m_watchModule(new WatchMemModule(this))
{
	ui->setupUi(this);
	//Mem watch
	ui->tableView_memstatus->setModel(m_watchModule);
	m_nTmid = startTimer(1000);
	m_DateAxix = new QtCharts::QDateTimeAxis;
	m_DateAxiy = new QtCharts:: QValueAxis;
	m_DateAxiy->setRange(0,256);
	m_DateAxix->setFormat("mm");
	ui->widget_memchart->chart()->addAxis(m_DateAxix, Qt::AlignBottom);
	ui->widget_memchart->chart()->addAxis(m_DateAxiy, Qt::AlignLeft);
}

FormStatus::~FormStatus()
{
	m_DateAxix->deleteLater();
	m_DateAxiy->deleteLater();
	delete ui;

}

void FormStatus::update_charts()
{
	if (m_watchModule)
	{
		QDateTime dtm = QDateTime::currentDateTime();
		m_watchModule->update_items();
		const int rows = m_watchModule->rowCount();
		QSet<qint64> abst ;
		int max_y = 0;
		for (int i=0;i<rows;++i)
		{
			const qint64 pid = m_watchModule->data(m_watchModule->index(i,0)).toLongLong();
			const QString name = m_watchModule->data(m_watchModule->index(i,1)).toString();
			abst.insert(pid);
			double mem = m_watchModule->data(m_watchModule->index(i,2)).toDouble();
			if (max_y<mem)
				max_y = mem;
			QtCharts::QLineSeries * ps  = nullptr;
			if (m_chart_serials.contains(pid)==false)
			{
				ps = new QtCharts::QLineSeries(this);
				m_chart_serials[pid] = ps;
				ui->widget_memchart->chart()->addSeries(ps);
				ps->setName(QString("%1").arg(pid));
				ps->attachAxis(m_DateAxix);
				ps->attachAxis(m_DateAxiy);
			}
			else
				ps = m_chart_serials[pid];			
			ps->append(dtm.toMSecsSinceEpoch(),mem);
			while (ps->count()>=1200)
				ps->remove(0);
		}
		//delete
		QList<qint64> oldV = m_chart_serials.keys();
		foreach(qint64 u, oldV)
		{
			if (abst.contains(u)==false)
			{
				ui->widget_memchart->chart()->removeSeries(m_chart_serials[u]);
				m_chart_serials[u]->deleteLater();
				m_chart_serials.remove(u);
			}
		}
		max_y *=1.1;
		m_DateAxiy->setMax(max_y);
		m_DateAxix->setRange(dtm.addSecs(-600),dtm);
		m_DateAxix->setTickCount(10);
	}
}
void FormStatus::timerEvent(QTimerEvent *event)
{
	if (m_nTmid==event->timerId())
		update_charts();
}
