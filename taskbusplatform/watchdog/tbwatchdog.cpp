#include "tbwatchdog.h"
#include "../core/process_prctl.h"
#include <QDebug>
#include <QTimerEvent>
#include <QVector>

static tbWatchDog * g_tb_watch_dog_ = nullptr;
tbWatchDog * tb_watch_dog()
{
	if (g_tb_watch_dog_==nullptr)
	{
		g_tb_watch_dog_ = new tbWatchDog;
	}
	return g_tb_watch_dog_;
}

tbWatchDog::tbWatchDog(QObject *parent) : QObject(parent)
{
	m_nTimerID = startTimer(1000);
}

void tbWatchDog::watch(QProcess * proc)
{
	Q_PID id = TASKBUS::get_procid(proc);
	if (id)
		m_set_pid.insert(id);
}


void tbWatchDog::timerEvent(QTimerEvent * evt)
{
	if (m_nTimerID==evt->timerId())
	{
		QVector<Q_PID> id_failed;
		foreach(Q_PID id, m_set_pid)
		{
			TASKBUS::tagMemoryInfo info;
			if (TASKBUS::get_memory(id,&info))
			{
				qDebug()<<info.m_name<<":"<<info.m_memsize<<"\n";
			}
			else
				id_failed << id;
		}
		foreach(Q_PID id, id_failed)
		{
			m_set_pid.remove(id);
		}
	}

}
