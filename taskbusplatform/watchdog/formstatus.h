#ifndef FORMSTATUS_H
#define FORMSTATUS_H

#include <QWidget>
#include <QLineSeries>
#include <QValueAxis>
#include <QDateTimeAxis>
#include "watchdog/watchmemmodule.h"
namespace Ui {
	class FormStatus;
}

class FormStatus : public QWidget
{
	Q_OBJECT

public:
	explicit FormStatus(QWidget *parent = nullptr);
	~FormStatus() override;
protected:
	void timerEvent(QTimerEvent *event) override;
private:
	Ui::FormStatus *ui;
	int m_nTmid = -1;
	WatchMemModule * m_watchModule = nullptr;
	QtCharts::QDateTimeAxis * m_DateAxix = nullptr;
	QtCharts::QValueAxis * m_DateAxiy = nullptr;
	QMap<qint64, QtCharts::QLineSeries *> m_chart_serials;
	void update_charts();
};

#endif // FORMSTATUS_H
