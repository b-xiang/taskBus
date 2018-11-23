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
	QMap<Q_PID, TASKBUS::tagMemoryInfo> m_map_pid;
	QMutex m_mutex;
};

tbWatchDog & tb_watch_dog();

#endif // TBWATCHDOG_H
