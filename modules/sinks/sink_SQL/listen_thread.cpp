#include "listen_thread.h"
#include "tb_interface.h"
#include <QByteArray>
#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QTextStream>
#include <QDateTime>
#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#endif
reciv_thread::reciv_thread(const TASKBUS::cmdlineParser * pCmd,QObject *parent)
	:QThread(parent)
	,m_pCmd(pCmd)
{

}

void reciv_thread::run()
{
	//连接数据库
	const std::string iDriver = m_pCmd->toString("DRIVER","QSQLITE");
	const std::string iHOST = m_pCmd->toString("HOST","");
	const std::string iPORT = m_pCmd->toString("PORT","");
	const std::string iDATABASE = m_pCmd->toString("DATABASE","./mydb.db");
	const std::string iUSER = m_pCmd->toString("USER","");
	const std::string iPASS = m_pCmd->toString("PASS","");
	const std::string istart_script = m_pCmd->toString("start_script","");

	if (istart_script.size())
	{
		std::string str_cmd = "start ";
		str_cmd += istart_script;
		system(str_cmd.c_str());
		QThread::msleep(1000);
	}


	const int iSQL = m_pCmd->toInt("sql",0);
	int iUseTrans = m_pCmd->toInt("use_transaction",0);
	const int iCommitTm = m_pCmd->toInt("commit_time",1);
	const int iencoding = m_pCmd->toInt("encoding",0);
	QTextStream st(stderr);
	QSqlDatabase db = QSqlDatabase::addDatabase(
				QString::fromStdString(iDriver),QString("DBSINK"));
	if (db.lastError().text().size())
	{
		st<<db.lastError().text();
	}
	if (iHOST.length()>1)
		db.setHostName(QString::fromStdString(iHOST));
	if (QString::fromStdString(iPORT).toInt()>0)
		db.setPort(QString::fromStdString(iPORT).toInt());
	if (iDATABASE.length()>1)
		db.setDatabaseName(QString::fromStdString(iDATABASE));
	if (iUSER.length()>1)
		db.setUserName(QString::fromStdString(iUSER));
	if (iPASS.length()>1)
		db.setPassword(QString::fromStdString(iPASS));

	if (db.open()==false)
	{
		st<<db.lastError().text();
		QSqlDatabase::removeDatabase("DBSINK");
		st.flush();
		return;
	}
	st.flush();

	bool bfinished = false;
	using namespace TASKBUS;
	QSqlQuery q(db);
	q.setForwardOnly(true);
	if (iUseTrans!=0)
	{
		if (false==db.transaction())
		{
			emit err_message(db.lastError().text());
			iUseTrans = 0;
		}
	}
	QDateTime dtmLastCmt = QDateTime::currentDateTime();
	while (false==bfinished)
	{
		subject_package_header header;
		std::vector<unsigned char> packagedta = pull_subject(&header);
		if (false==is_valid_header(header))
		{
			msleep(20);
			continue;
		}
		if ( is_control_subject(header) && packagedta.size())
		{
			//收到命令进程退出的广播消息,退出
			if (strstr(control_subject(header,packagedta).c_str(),"\"quit\":"))
			{
				bfinished = true;
				qDebug()<<"Quit!";
				emit sig_quit();
			}
		}
		else if (header.subject_id==iSQL)
		{
			packagedta.push_back(0);
			QString strSql;
			if (iencoding==0)
				strSql = QString::fromLocal8Bit((char *)packagedta.data());
			else
				strSql = QString::fromUtf8((char *)packagedta.data());
			try{
				if (q.exec(strSql)==false)
					throw q.lastError();
				++m_nTotalOK;
				if (m_nTotalOK%100==0 && iUseTrans==0)
					emit new_message(QString("%1 Items inserted.").arg(m_nTotalOK));
			}
			catch(QSqlError e)
			{
				emit err_message(e.text()+">"+strSql);
			}
		}
		if(iUseTrans!=0)
		{
			QDateTime dtmCurr = QDateTime::currentDateTime();
			if(dtmLastCmt.secsTo(dtmCurr)>iCommitTm)
			{
				dtmLastCmt = dtmCurr;
				if(false==db.commit())
				{
					emit err_message(db.lastError().text());
					db.rollback();
					iUseTrans = 0;
				}
				else if (iUseTrans==0)
					emit new_message(QString("%1 Items inserted.").arg(m_nTotalOK));

			}
		}

	}

	return ;
}

