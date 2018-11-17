/*!
  * 这个文件为不同的平台提供统一的进程优先级设置接口。
  * This file provides a unified Process priority setting interface for
  * different platforms.
  */
#ifndef PROCESS_PRCTL_H
#define PROCESS_PRCTL_H
class QProcess;
namespace TASKBUS {
	void set_proc_nice (QProcess * p, int nice);
	void set_proc_nice (int nice);

	extern const int pnice_min;
	extern const int pnice_max;
	extern const int pnice_norm;

}
#endif // PROCESS_PRCTL_H
