﻿/*!
  @author goldenhawking@163.com
  @date 2016-09-12
  */
#include "tasknode.h"
#include <QMutex>
#include <QThread>
#include <QTextStream>
#include <QCoreApplication>
#include <QDir>
#include <QDebug>
#include <QAtomicInt>
#include "tb_interface.h"
#include "process_prctl.h"
#include "../watchdog/tbwatchdog.h"
#include "../watchdog/profile_log.h"
taskNode::taskNode(QObject *parent)
	: QObject(parent)
	,m_process(new QProcess(this))
{
	connect(m_process,&QProcess::started,this,
			&taskNode::sig_pro_started,Qt::QueuedConnection);
	connect(m_process,&QProcess::started,this,
			&taskNode::slot_started,Qt::QueuedConnection);
	connect(m_process,
			static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>
			(&QProcess::finished),
			this,&taskNode::sig_pro_stopped,Qt::QueuedConnection);
	connect(m_process,
			static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>
			(&QProcess::finished),
			this,&taskNode::slot_stopped,Qt::QueuedConnection);
	connect(m_process,&QProcess::readyReadStandardOutput,this,
			&taskNode::slot_readyReadStandardOutput,Qt::QueuedConnection);
	connect(m_process,&QProcess::readyReadStandardError,this,
			&taskNode::slot_readyReadStandardError,Qt::QueuedConnection);
	connect(m_process,&QProcess::bytesWritten,this,
			&taskNode::slot_sended,Qt::QueuedConnection);
	connect(this,&taskNode::private_sig_nextcab,this,
			&taskNode::slot_readyReadStandardOutput,Qt::QueuedConnection);
	m_nBp_TimerID = startTimer(200);
}
taskNode::~taskNode()
{
	cmd_stop(this);
}

/*!
 * \brief taskNode::cmd_start 开启进程进行工作。Start the process to work.
 * \param node   信号发送者希望开启的taskNode实例。
 * The Tasknode instance that the signal sender wants to open.
 * \param cmd    命令行 Commandline filename
 * \param paras  参数表 Commandline parameters
 * \return 成功、失败   True when succeed
 */
bool taskNode::cmd_start(QObject * node,QString cm, QStringList paras)
{
	if (node !=this)
		return false;
	if (m_process->state()!=QProcess::NotRunning)
		return false;

	const QString cmdline =cm;
	QStringList lstCmds = paras;
	m_process->setProgram(cmdline);
	m_process->setArguments(lstCmds);
	m_process->start();

	QString cmdlinestr = cmdline;
	foreach (QString cmd, lstCmds)
		cmdlinestr += " " + cmd;

	emit_message(QByteArray(cmdlinestr.toStdString().c_str()));

	m_sbytes_sent = 0;
	m_spackage_sent= 0;
	m_sbytes_recieved = 0;
	m_spackage_recieved = 0;
	return true;
}


bool taskNode::cmd_stop(QObject * node)
{
	using namespace TASKBUS;
	if (node !=this)
		return false;
	if (m_process->state()!=QProcess::Running)
		return false;
	//发送信令，终止
	char cmd[] = "function=quit;ret=0;source=taskbus;destin=all;";
	subject_package_header header;
	header.prefix[0] = 0x3C;
	header.prefix[1] = 0x5A;
	header.prefix[2] = 0x7E;
	header.prefix[3] = 0x69;
	header.data_length = static_cast<unsigned int>(strlen(cmd)+1);
	header.path_id = 1;
	header.subject_id = control_subect_id();
	m_process->write((char *)&header,sizeof(subject_package_header));
	m_process->write(cmd,strlen(cmd)+1);
	int c = 0;
	while (++c<100 && m_process->state()==QProcess::Running)
	{
		m_process->waitForFinished(20);
		QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
	}
	m_process->kill();

	return true;
}

void taskNode::slot_started()
{
	if (m_bDebug)
	{
		QString strDebugDir = dbgdir();
		m_dbgfile_stderr.close();
		m_dbgfile_stdin.close();
		m_dbgfile_stdout.close();

		m_dbgfile_stderr.setFileName(strDebugDir+"/stderr.txt");
		m_dbgfile_stdin.setFileName(strDebugDir+"/stdin.dat");
		m_dbgfile_stdout.setFileName(strDebugDir+"/stdout.dat");

		if (m_dbgfile_stderr.open(QIODevice::WriteOnly)==true)
		{
			QTextStream st(&m_dbgfile_stderr);
			st<<m_process->arguments().size()+1<<"\n";
			st<<m_process->program();
			foreach (QString a,m_process->arguments())
				st<<"\n"<<a;
			st<<"\n";
			st.flush();
		}
		m_dbgfile_stdout.open(QIODevice::WriteOnly);
		m_dbgfile_stdin.open(QIODevice::WriteOnly);

	}

	if (m_pCell)
	{
		QMap<QString,QVariant> vm
				= m_pCell->additional_paras(m_pCell->function_firstname());
		if (vm.contains("nice"))
		{
			int nic = vm["nice"].toInt();
			TASKBUS::set_proc_nice(m_process,nic);
		}
	}

	tb_watch_dog().watch(m_process);

}

void taskNode::slot_stopped()
{

}
/*!
 * \brief taskNode::slot_readyReadStandardOutput
 * 为了后续的处理需要，这里会等到一个完整的包到来再发送。
 * For subsequent processing needs,
 * this will wait until a complete package arrives before sending.
 */
void taskNode::slot_readyReadStandardOutput()
{
	if (m_bBP_blocked)
		return;
	LOG_PROFILE("IO","Start Recieving packs.");
	QByteArray arred = m_process->readAllStandardOutput();
	m_array_stdout.append(arred);
	extern QAtomicInt  g_totalrev;
	g_totalrev += arred.size();
	while (m_array_stdout.size()>=static_cast<int>(sizeof(TASKBUS::subject_package_header)))
	{
		auto * header =
				reinterpret_cast<const TASKBUS::subject_package_header *>
				(m_array_stdout.constData());
		//合法性 Legitimacy
		if (header->prefix[0]!=0x3C || header->prefix[1]!=0x5A ||
				header->prefix[2]!=0x7E || header->prefix[3]!=0x69)
		{
			emit_message(QByteArray("Error header recieved. "
										   "Header must be 0x3C, 0x5A, 0x7E,"
										   " 0x69. Aborting."));
			m_array_stdout.clear();
		}
		else
		{
			if (static_cast<size_t>(m_array_stdout.size())>=
					sizeof(TASKBUS::subject_package_header)+header->data_length)
			{
				++m_spackage_sent;
				m_sbytes_sent += sizeof(TASKBUS::subject_package_header)+header->data_length;

				QByteArray arr(m_array_stdout.constData(),
							   sizeof(TASKBUS::subject_package_header)
							   +header->data_length);
				m_array_stdout.remove(0,sizeof(TASKBUS::subject_package_header)
									  +header->data_length);
				//Analyse cab.
				const TASKBUS::subject_package_header * header_package =
						reinterpret_cast<const TASKBUS::subject_package_header *>(arr.constData());
				//Command
				if (header_package->subject_id == 0xffffffff)
				{
					//Command must endwith \0
					const char * pCmd = arr.constData()+sizeof(TASKBUS::subject_package_header);
					QString cmd = QString::fromUtf8(pCmd,header_package->data_length);
					QMap<QString, QVariant> map_z
							= taskCell::string_to_map(cmd);
					//remember uuid
					if (map_z.contains("source"))
					{
						if(m_uuid.size()==0 )
							m_uuid = map_z["source"].toString();
						if (map_z.contains("destin"))
							emit sig_new_command(map_z);
					}
				}
				else
					emit_package(arr);
				if (m_bDebug)
					log_package(true,arr);
			}
			//处理了一个包之后，若还存在后续的包，则继续处理。
			//After processing a package, continue processing if a subsequent
			// package is present.
			if (static_cast<size_t>(m_array_stdout.size())>=sizeof(TASKBUS::subject_package_header))
			{
				header =
						reinterpret_cast<const TASKBUS::subject_package_header *>(m_array_stdout.constData());
				if (!(static_cast<size_t>(m_array_stdout.size())>=sizeof(TASKBUS::subject_package_header)+header->data_length))
					break;
			}
		}
	}
	LOG_PROFILE("IO","End Recieving packs.");
}

/*!
 * \brief taskNode::emit_message
 * Signals and Slots is easy to use, but it will take too much cpu resource with out bufferring .
 * 没有缓存机制，频繁的发送信号，将降低系统的性能。
 * We will check the timestamp between 2 signals, catch it , and send it out as a batch array.
 * \param arred single message.
 */
void taskNode::emit_message(QByteArray arred)
{
	static clock_t last_ck = clock();
	//Prevent of too short freq.
	clock_t curr_ck = clock();
	bool keep = false;
	if (curr_ck-last_ck <=CLOCKS_PER_SEC/10 && curr_ck-last_ck>=0)
		keep = true;
	last_ck = curr_ck;
	m_arr_Strerr.push_back(arred);
	if (false==keep)
		flush_from_stderr();
}

/*!
 * \brief taskNode::emit_package
 * * Signals and Slots is easy to use, but it will take too much cpu resource with out bufferring .
 * 没有缓存机制，频繁的发送信号，将降低系统的性能。
 * We will check the timestamp between 2 signals, catch it , and send it out as a batch list.
 * \param package
 */
void taskNode::emit_package(QByteArray package)
{
	static clock_t last_ck = clock();
	//Prevent of too short freq.
	clock_t curr_ck = clock();
	bool keep = false;
	if (curr_ck-last_ck <=CLOCKS_PER_SEC/10 && curr_ck-last_ck>=0)
		keep = true;
	last_ck = curr_ck;
	m_packBuf.push_back(package);
	if (keep==false)
		flush_from_stdout();
}

void taskNode::flush_from_stderr()
{
	if (m_arr_Strerr.size())
	{
		emit sig_new_errmsg(m_arr_Strerr);
		m_arr_Strerr.clear();
	}
}
void taskNode::flush_from_stdout()
{
	if (m_packBuf.size())
	{
		emit sig_new_package(m_packBuf);
		m_packBuf.clear();
	}
}

int taskNode::outputQueueSize()
{
	return m_nBp_QueueSz;
}

void taskNode::setBlockFlag(bool b)
{
	if (b==false && m_bBP_blocked==true && m_bBp_Recovered==false)
		m_bBp_Recovered = true;
	m_bBP_blocked = b;

}
void taskNode::timerEvent(QTimerEvent *event)
{
	if (event->timerId()==m_nBp_TimerID)
	{
		static int ct = 0;
		if (m_bBp_Recovered)
		{
			if (m_process->bytesAvailable())
				slot_readyReadStandardOutput();
		}
		if (++ct % 3 ==0)
		{
			if (isRunning())
			{
				qint64 pid = TASKBUS::get_procid(m_process);
				emit sig_iostat(pid,m_spackage_recieved,m_spackage_sent,m_sbytes_recieved,	m_sbytes_sent);
			}
		}
		flush_from_stderr();
		flush_from_stdout();
	}
}




/*!
 * \brief taskNode::log_package 记录调试信息 Logging Debug Information
 * \param fromStdOut 是从进程的stdout来的信息 Is the information from the
 *                   stdout of the process
 * \param arrPackage 包
 */
void taskNode::log_package(bool fromStdOut,QByteArray arrPackage)
{
	if (m_bDebug)
	{
		if (fromStdOut && m_dbgfile_stdout.isOpen())
		{
			m_dbgfile_stdout.write(arrPackage);
			m_dbgfile_stdout.flush();
		}
		else if (fromStdOut==false && m_dbgfile_stdin.isOpen())
		{
			m_dbgfile_stdin.write(arrPackage);
			m_dbgfile_stdin.flush();

		}
	}

}

void taskNode::slot_readyReadStandardError()
{
	QByteArray arred =m_process->readAllStandardError();
	extern QAtomicInt g_totalrev;
	g_totalrev += arred.size();

	emit_message(arred);

	if (m_bDebug && m_dbgfile_stderr.isOpen())
	{
		QTextStream st(&m_dbgfile_stderr);
		st<<arred;
		st.flush();
	}

}
bool taskNode::cmd_write(QObject * node,QByteArray arr)
{
	if (node !=this)
		return false;
	if (m_bDebug)
		log_package(false,arr);
	if (m_process->state()!=QProcess::Running)
		return false;
	if (m_outputBuf.size()==0)
		m_process->write(arr);
	else
		m_outputBuf.push_back(arr);
	m_nBp_QueueSz = m_outputBuf.size();
	++m_spackage_recieved;
	m_sbytes_recieved += arr.size();
	return  true;
}

bool taskNode::cmd_sendcmd(QMap<QString,QVariant> cmd, QSet<QString> destins)
{
	if (destins.contains(m_uuid)==false
			&&destins.contains("all")==false)
		return false;
	if (cmd.contains("source"))
		if (cmd["source"]==m_uuid)
			return false;
	QString strv = taskCell::map_to_string(cmd);
	QByteArray utf8 = strv.toUtf8();
	QByteArray arr;
	arr.append(0x3C);	arr.append(0x5A);
	arr.append(0x7E);	arr.append(0x69);
	arr.append(4,0xFFu); arr.append(4,0x00u);
	utf8.append('\0');
	const int sz = utf8.size();
	arr.append((sz>> 0) & 0xff);
	arr.append((sz>> 8) & 0xff);
	arr.append((sz>> 16) & 0xff);
	arr.append((sz>> 24) & 0xff);
	arr.append(utf8);
	return cmd_write(this,arr);
}

void taskNode::slot_sended(qint64 b)
{
	extern QAtomicInt g_totalsent ;
	g_totalsent += b;
	if (m_outputBuf.size())
	{
		m_process->write(m_outputBuf.first());
		m_outputBuf.pop_front();
	}
	m_nBp_QueueSz = m_outputBuf.size();
}
bool taskNode::isRunning ()
{
	return m_process->state()==QProcess::NotRunning?false:true;
}

QString taskNode::dbgdir()
{
	QString str = QCoreApplication::applicationDirPath() + "/debug";
	if (!m_process)
		return str;
	str +=  QString("/pid%1").arg(m_process->processId());
	QDir dir;
	dir.mkpath(str);
	return str;
}
