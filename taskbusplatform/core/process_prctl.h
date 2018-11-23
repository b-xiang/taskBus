/*!
  * 这个文件为不同的平台提供统一的进程设置接口。
  * This file provides a unified Process setting interface for
  * different platforms.
  */
#ifndef PROCESS_PRCTL_H
#define PROCESS_PRCTL_H
#include <QProcess>
namespace TASKBUS {
	void set_proc_nice (QProcess * p, int nice);
	void set_proc_nice (int nice);

	qint64 get_procid(QProcess * p);

	struct tagMemoryInfo{
		qint64 pid;
		QString m_name;
		double m_memsize;

	};

	bool get_memory (qint64 p ,tagMemoryInfo * info);

	extern const int pnice_min;
	extern const int pnice_max;
	extern const int pnice_norm;

}
#endif // PROCESS_PRCTL_H
