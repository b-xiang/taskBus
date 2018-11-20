#ifndef TBWATCHDOG_H
#define TBWATCHDOG_H

#include <QObject>
#include <QSet>
#include "../core/taskproject.h"

class tbWatchDog : public QObject
{
	Q_OBJECT
public:
	explicit tbWatchDog(QObject *parent = nullptr);
	void watch(taskProject * project);
	void unwatch(taskProject * project);
	void unwatch_all();

protected:
	void timerEvent(QTimerEvent * evt) override;
signals:

public slots:


private:
	QSet<taskProject *> m_set_projects;
};

#endif // TBWATCHDOG_H
