#include "process_prctl.h"
#include <QFile>
#include <QTextStream>
#ifdef WIN32
#include <windows.h>
#endif
#ifdef linux
#include <sys/resource.h>
#include <unistd.h>
#endif

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

	bool get_memory (Q_PID p ,tagMemoryInfo * info)
	{
#ifdef linux
		info->pid = p;
		QString strFm;
		strFm.sprintf("/proc/%lld/status",p);
		QFile fin(strFm);
		if (fin.open(QIODevice::ReadOnly)==false)
			return false;
		QTextStream s(&fin);
		int hit = 0;
		QString l = s.readLine();
		while (l.size() && hit<2)
		{			
			QStringList lst = l.split(":",QString::SkipEmptyParts);
			if (lst.size()>=2)
			{
				QString keystr = lst.first().trimmed();
				lst.pop_front();
				QString v = lst.first().trimmed();
				if (keystr=="Name")
				{
					info->m_name = v;
					++hit;
				}
				if (keystr=="VmRSS")
				{
					v = v.toUpper();
					++hit;
					if (v.indexOf("K"))
					{
						QString digiga = v.left(v.indexOf("K"));
						info->m_memsize = digiga.toDouble()*1024;
					}
					else if (v.indexOf("M"))
					{
						QString digiga = v.left(v.indexOf("M"));
						info->m_memsize = digiga.toDouble()*1024*1024;
					}
					else if (v.indexOf("G"))
					{
						QString digiga = v.left(v.indexOf("G"));
						info->m_memsize = digiga.toDouble()*1024*1024*1024;
					}
					else if (v.indexOf("B"))
					{
						QString digiga = v.left(v.indexOf("B"));
						info->m_memsize = digiga.toDouble();
					}
					else if (v.toDouble()>0)
						info->m_memsize = v.toDouble();
					else
						--hit;
				}

			}
			l = s.readLine();
		}
		fin.close();
		if (hit>=2)
			return true;
#endif
		return false;
	}

	Q_PID get_procid(QProcess * p)
	{
		if (p)
			return p->pid();
#ifdef linux
		Q_PID id = getpid();
		return id;
#endif
#ifdef WIN32

#endif
		return 0;
	}
}
