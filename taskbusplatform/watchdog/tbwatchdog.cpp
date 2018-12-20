#include "tbwatchdog.h"
#include <QDebug>
#include <QTimerEvent>
#include <QVector>
static bool g_tb_break = false;
#ifdef WIN32
#include <windows.h>
BOOL HandlerRoutine(DWORD dwCtrlType)
{
	switch (dwCtrlType)
	{
	case CTRL_C_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_BREAK_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		g_tb_break = true;
		return TRUE;
	default:
		return FALSE;
	}
}
#endif
#ifdef linux
#include <unistd.h>
#include <signal.h>
void my_handler(int s){
	fprintf(stderr,"Caught signal %d\n",s);
	fflush(stderr);
	g_tb_break = true;
}
#endif

static tbWatchDog g_tb_watch_dog_;

tbWatchDog::tbWatchDog()
{
#ifdef WIN32
	static bool handle_installed = false;
	if (handle_installed==false)
	{
		SetConsoleCtrlHandler((PHANDLER_ROUTINE)HandlerRoutine,TRUE);
		handle_installed = true;
	}
#endif

#ifdef linux
	struct sigaction sigIntHandler;

	sigIntHandler.sa_handler = my_handler;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;
	sigaction(SIGINT, &sigIntHandler, NULL);
#endif
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

bool tbWatchDog::break_hit()
{
	return g_tb_break;
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

