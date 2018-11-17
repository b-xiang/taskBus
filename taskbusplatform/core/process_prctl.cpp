#include <QProcess>
#ifdef WIN32
#include <windows.h>
#endif

#ifdef linux
#include <sys/resource.h>
#include <unistd.h>
#endif
#include "process_prctl.h"

namespace TASKBUS {

#ifdef WIN32
	const int pnice_min = 0;
	const int pnice_max = 5;
	const int pnice_norm = 2;
#endif

#ifdef linux
	const int pnice_min = PRIO_MIN;
	const int pnice_max = PRIO_MAX;
	const int pnice_norm = 0;

#endif

	void set_proc_nice(QProcess * p, int nice)
	{
	#ifdef WIN32
		Q_PID id = p->pid();
		const DWORD dw_pn []= {
			IDLE_PRIORITY_CLASS,
			BELOW_NORMAL_PRIORITY_CLASS,
			NORMAL_PRIORITY_CLASS,
			ABOVE_NORMAL_PRIORITY_CLASS,
			HIGH_PRIORITY_CLASS,
			REALTIME_PRIORITY_CLASS
		};
		if (nice<0)		nice = 0;
		if (nice>5)		nice = 5;
		SetPriorityClass(id->hProcess,dw_pn[nice]);
	#endif

#ifdef linux
		Q_PID id = p->pid();
		if (nice<-PRIO_MIN)	nice = PRIO_MIN;
		if (nice>PRIO_MAX)	nice = PRIO_MAX;
		setpriority(PRIO_PROCESS,id,nice);
#endif


	}

	void set_proc_nice (int nice)
	{
	#ifdef WIN32
		const DWORD dw_pn []= {
			IDLE_PRIORITY_CLASS,
			BELOW_NORMAL_PRIORITY_CLASS,
			NORMAL_PRIORITY_CLASS,
			ABOVE_NORMAL_PRIORITY_CLASS,
			HIGH_PRIORITY_CLASS,
			REALTIME_PRIORITY_CLASS
		};
		if (nice<0)		nice = 0;
		if (nice>5)		nice = 5;
		SetPriorityClass(GetCurrentProcess(),dw_pn[nice]);
	#endif

#ifdef linux
		Q_PID id = getpid();
		if (nice<-PRIO_MIN)	nice = PRIO_MIN;
		if (nice>PRIO_MAX)	nice = PRIO_MAX;
		setpriority(PRIO_PROCESS,id,nice);
#endif
	}

}
