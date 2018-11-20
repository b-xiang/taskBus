#include "tbwatchdog.h"
#include <QTimerEvent>
tbWatchDog::tbWatchDog(QObject *parent) : QObject(parent)
{

}

void tbWatchDog::watch(taskProject * project)
{
	m_set_projects.insert(project);
}
void tbWatchDog::unwatch(taskProject * project)
{
	m_set_projects.remove(project);
}
void tbWatchDog::unwatch_all()
{
	m_set_projects.clear();
}

void tbWatchDog::timerEvent(QTimerEvent * evt)
{

}
