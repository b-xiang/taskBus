/*!
  * taskNode用来管理各个功能对应的进程。taskNode实例与一个进程一一对应，负责直接管理
  * 进程启动、终止、数据组合与吞吐。该类从QObject派生，将在taskProject的管理下，
  * 运行在线程池中。
  * Tasknode is used to manage the processes that correspond to each Function.
  * The Tasknode instance corresponds to a process one by one, which is
  * responsible for directly managing the process initiation, termination,
  * data combination, and throughput. The class is derived from Qobject and
  * will run in the thread pool under taskproject management.
  * @author goldenhawking@163.com, 2016-09
  */
#ifndef TASKNODE_H
#define TASKNODE_H

#include <QObject>
#include "taskcell.h"
#include <QProcess>
#include <QList>
#include <QByteArrayList>
#include <QFile>
class taskCell;
/*!
 * \brief The taskNode class 管理一个构件进程的类
 * 这个类包含基本的进程启动终止、管道维护
 * A class that manages a component process. This class contains basic process
 *  start-up termination, pipeline maintenance
 */
class taskNode : public QObject
{
	Q_OBJECT
public:
	explicit taskNode( QObject *parent = nullptr);
	~taskNode();
signals:
	//进程消息 Process messages
	void sig_pro_started();
	void sig_pro_stopped(int exitCode, QProcess::ExitStatus exitStatus);
	void sig_new_package(QByteArrayList);
	void sig_new_command(QMap<QString, QVariant> cmd);
	void sig_new_errmsg(QByteArrayList);
	void sig_iostat(qint64 pid,quint64 pr,quint64 ps,quint64 br, quint64 bs);
	void private_sig_nextcab();
	void private_sig_nextwrite();
public slots:
	void setDebug(bool bdbg){m_bDebug = bdbg;}
	bool cmd_start(QObject * node,QString cmd, QStringList paras);
	bool cmd_stop(QObject * node);
	bool cmd_write(QObject * node,QByteArray arr);
	bool cmd_sendcmd(QMap<QString,QVariant> cmd, QSet<QString> destins);
private slots:
	void slot_readyReadStandardOutput();
	void slot_readyReadStandardError();
	void slot_sended(qint64 );
	void slot_started( );
	void slot_stopped();
private:
	QProcess * m_process = nullptr;
	//进程缓存
	QByteArray m_array_stdout;
	QByteArrayList m_outputBuf;
	QByteArrayList m_arr_Strerr;
	QByteArrayList m_packBuf;
	bool m_bDebug = false;
	//Flush buffered messages, emit signals.
	void flush_from_stderr();
	void flush_from_stdout();
private:
	void log_package(bool fromStdOut,QByteArray arrPackage);
	QFile m_dbgfile_stdin;
	QFile m_dbgfile_stdout;
	QFile m_dbgfile_stderr;
	QString m_uuid;
	QString dbgdir();
	//Call these functions to emit message and packs with bufferring approach.
	void emit_message(QByteArray a);
	void emit_package(QByteArray packages);
public:
	int outputQueueSize();
	void setBlockFlag(bool b);
protected:
	bool m_bBP_blocked = false;
	bool m_bBp_Recovered = false;
	int  m_nBp_QueueSz = 0;
	int  m_nBp_TimerID = -1;
	taskCell * m_pCell = nullptr;
	void timerEvent(QTimerEvent *event);
	//statistic
protected:
	quint64 m_spackage_recieved = 0;
	quint64 m_spackage_sent = 0;
	quint64 m_sbytes_recieved = 0;
	quint64 m_sbytes_sent = 0;
public:
	bool		isRunning ();
	QString		uuid()const {return m_uuid;}
	bool		isDebug() const {return m_bDebug;}
	QProcess *	proc(){return m_process;}
	void		bindCell(taskCell * t){m_pCell = t;}
	taskCell *	cell(){return m_pCell;}
	quint64 st_pack_recieved() const {return m_spackage_recieved;}
	quint64 st_pack_sended() const {return m_spackage_sent;}
	quint64 st_bytes_recieved() const {return m_sbytes_recieved;}
	quint64 st_bytes_sended() const {return m_sbytes_sent;}
};

#endif // TASKNODE_H
