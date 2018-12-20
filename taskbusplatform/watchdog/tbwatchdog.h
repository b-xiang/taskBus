/*!
 * Watchdog for memory size monitor abd  Handle console events for ^C
  * ^C will interrupt current process, module processes will be left
  * behind , and become un-controlled.
  * 这个工具函数文件用来处理控制台终止信号 ^C ，
  * 在关闭时，若不首先级联关闭其他子模块，则子模块会滞留，无法控制。
  @author goldenhawking@163.com
  @date 2016-09-12
*/
#ifndef TBWATCHDOG_H
#define TBWATCHDOG_H

#include <QAbstractTableModel>
#include <QMap>
#include <QVector>
#include <QProcess>
#include <QMutex>
#include "../core/taskproject.h"
#include "../core/process_prctl.h"
class tbWatchDog
{
public:
	tbWatchDog();
	void watch(QProcess * proc = nullptr);
	void update_table();
	QVector<TASKBUS::tagMemoryInfo> get_info();
private:
	QMap<qint64, TASKBUS::tagMemoryInfo> m_map_pid;
	QMutex m_mutex;
public:
	static bool break_hit();
};

tbWatchDog & tb_watch_dog();

#endif // TBWATCHDOG_H
