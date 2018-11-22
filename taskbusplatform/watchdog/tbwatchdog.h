#ifndef TBWATCHDOG_H
#define TBWATCHDOG_H

#include <QObject>
#include <QSet>
#include <QProcess>
#include "../core/taskproject.h"

class tbWatchDog : public QObject
{
	Q_OBJECT
public:
	explicit tbWatchDog(QObject *parent = nullptr);

protected:
	void timerEvent(QTimerEvent * evt) override;
signals:

public slots:
	void watch(QProcess * proc = nullptr);

private:
	QSet<Q_PID> m_set_pid;

	int m_nTimerID = 0;
};

tbWatchDog * tb_watch_dog();

#endif // TBWATCHDOG_H
