#include "consolewatchdog.h"
#include <QTimerEvent>
#ifdef WIN32
extern bool g_tb_break;
#endif

consoleWatchDog::consoleWatchDog(QObject *parent)
	: QObject(parent)
	, tbWatchDog()
{
	m_nTimerID = startTimer(200);
}

void consoleWatchDog::timerEvent(QTimerEvent *event)
{
	if (event->timerId()==m_nTimerID)
	{
		if (tbWatchDog::break_hit()==true)
		{
			killTimer(m_nTimerID);
			m_nTimerID = -1;
			emit sig_shutdown();
		}
	}
}
