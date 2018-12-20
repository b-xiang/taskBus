#ifndef CONSOLEWATCHDOG_H
#define CONSOLEWATCHDOG_H

#include <QObject>
#include "tbwatchdog.h"
class consoleWatchDog : public QObject, public tbWatchDog
{
	Q_OBJECT
public:
	explicit consoleWatchDog(QObject *parent = nullptr);
protected:
	void timerEvent(QTimerEvent *event);
signals:
	void sig_shutdown();
private:
	int m_nTimerID = -1;
};

#endif // CONSOLEWATCHDOG_H
