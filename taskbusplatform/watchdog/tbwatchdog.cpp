#include "tbwatchdog.h"

#include <QDebug>
#include <QTimerEvent>
#include <QVector>

static tbWatchDog g_tb_watch_dog_;

tbWatchDog::tbWatchDog()
{

}

void tbWatchDog::watch(QProcess * proc)
{
	m_mutex.lock();
	qint64 id = TASKBUS::get_procid(proc);
	if (id)
	{
		TASKBUS::tagMemoryInfo info;
		TASKBUS::get_memory(id,&info);
		m_map_pid[id] = info;
	}
	m_mutex.unlock();
}


void tbWatchDog::update_table()
{
	m_mutex.lock();
	QList<qint64> ids = m_map_pid.keys();
	foreach(qint64 id, ids)
	{
		TASKBUS::tagMemoryInfo info;
		if (TASKBUS::get_memory(id,&info))
			m_map_pid[id] = info;
		else
			m_map_pid.remove(id);
	}
	m_mutex.unlock();

}

QVector<TASKBUS::tagMemoryInfo> tbWatchDog::get_info()
{
	QVector<TASKBUS::tagMemoryInfo> lstInfo;
	m_mutex.lock();
	QList<qint64> ids = m_map_pid.keys();
	foreach(qint64 id, ids)
	{
		lstInfo << m_map_pid[id];
	}
	m_mutex.unlock();
	return  lstInfo;
}

tbWatchDog & tb_watch_dog()
{
	return g_tb_watch_dog_;
}

