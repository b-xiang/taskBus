#include "tbwatchdog.h"

#include <QDebug>
#include <QTimerEvent>
#include <QVector>

static tbWatchDog * g_tb_watch_dog_ = nullptr;

tbWatchDog::tbWatchDog()
{

}

void tbWatchDog::watch(QProcess * proc)
{
	m_mutex.lock();
	Q_PID id = TASKBUS::get_procid(proc);
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
	QList<Q_PID> ids = m_map_pid.keys();
	foreach(Q_PID id, ids)
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
	QList<Q_PID> ids = m_map_pid.keys();
	foreach(Q_PID id, ids)
	{
		lstInfo << m_map_pid[id];
	}
	m_mutex.unlock();
	return  lstInfo;
}

tbWatchDog * tb_watch_dog()
{
	if (g_tb_watch_dog_==nullptr)
	{
		g_tb_watch_dog_ = new tbWatchDog();
	}
	return g_tb_watch_dog_;
}

