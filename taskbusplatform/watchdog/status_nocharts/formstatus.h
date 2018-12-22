#ifndef FORMSTATUS_H
#define FORMSTATUS_H

#include <QWidget>
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
	WatchMemModule * wmod() {return m_watchModule;}
	void clearMap(){}
protected:
	void timerEvent(QTimerEvent *event) override;
private:
	int m_nTmid = -1;
	Ui::FormStatus *ui;
	WatchMemModule * m_watchModule = nullptr;
	void update_charts();
};

#endif // FORMSTATUS_H
