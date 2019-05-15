/**
  Class profile_log is a lite tool-kit for millsec multi-thread profile log.
  @author goldenhawking@163.com
  @date 2019-05-14
  */
#ifndef PROFILE_LOG_H
#define PROFILE_LOG_H
#include <QDir>
#include <QFile>
#include <QIODevice>
#include <QTextStream>
#include <QThread>
#include <QString>
#include <QDateTime>
#include <QMutex>
#include <QUuid>
#include <QCoreApplication>
#include <memory>

#define LOG_PROFILE(SUBJECT,DETAILED) profile_log::log(\
	SUBJECT,DETAILED,__FILE__,__LINE__,__FUNCTION__)

/*!
	 * \brief The profile_log class is a tool-class for lite profile log.
	 * We can use this tool-kit simply by 3 steps:
	 * 1. include profile_log.h
	 * 2. Call profile_log::init at the very beginning of your program.
	 * 3. Call LOG_PROFILE(Subject, Detailed) anywhere you want.
	 */
class profile_log{
public:
	static inline bool init()
	{
		if (instance().get()!=nullptr)
			return false;
		instance() = std::shared_ptr<profile_log>(new profile_log());
		return instance()->write_title();
	}
	static inline bool init(const QString & filename)
	{
		if (instance().get()!=nullptr)
			return false;
		instance() = std::shared_ptr<profile_log>(new profile_log(filename));
		return instance()->write_title();
	}
	static inline bool init(QIODevice * dev)
	{
		if (instance().get()!=nullptr)
			return false;
		instance() = std::shared_ptr<profile_log>(new profile_log(dev));
		return instance()->write_title();
	}
	static inline std::shared_ptr<profile_log> & instance()
	{
		static std::shared_ptr<profile_log> plog;
		return plog;
	}
	static inline bool log_state()
	{
		if (!instance().get()) return false;
		return instance()->m_bLogOn;
	}
	static inline bool set_log_state(bool s)
	{
		if (!instance().get()) return false;
		return instance()->m_bLogOn = s;
	}
	static QString url(){
		if (!instance().get()) return "";
		return instance()->m_url;
	}
protected:
	profile_log(){
		//m_url = QDir::tempPath()+"/"+QUuid::createUuid().toString()+".csv";
		m_url = QCoreApplication::applicationDirPath()+"/log/";
		QDir dir;
		dir.mkpath(m_url);
		m_url += "/" + QUuid::createUuid().toString()+".csv";
		QFile * fp  = new QFile(m_url);
		if(fp->open(QIODevice::WriteOnly))
		{
			m_pDev = fp;
			m_bOwnDev = true;
		}
	}
	profile_log(const QString & filename){
		QFile * fp  = new QFile(filename);
		if(fp->open(QIODevice::WriteOnly))
		{
			m_pDev = fp;
			m_bOwnDev = true;
			m_url = filename;
		}
	}
	profile_log(QIODevice * dev){
		m_pDev = dev;
		QFileDevice * fp = qobject_cast<QFileDevice *>(dev);
		if (fp)
			m_url = fp->fileName();
		m_bOwnDev = false;
	}
public:
	~profile_log()
	{
		if (m_bOwnDev && m_pDev)
		{
			if (m_pDev->isOpen())
				m_pDev->close();
			m_pDev->deleteLater();
		}
		if (!m_bLogOn)
			if (m_url.length())
				QFile::remove(m_url);


	}
	static inline bool write_title()
	{
		if (!instance().get()) return false;
		if (instance()->log_state()==false)
			return true;
		instance()->m_mutex.lock();
		QTextStream st_out(instance()->m_pDev);
		st_out<<"Subject,Detailed,FileName,LineNum,FunctionName,Thread,UTC,Clock\n";
		st_out.flush();
		instance()->m_mutex.unlock();
		return true;
	}
	static inline bool log(const QString & subject, const QString & detailed,
						   const QString & filename,
						   const int linenum,
						   const QString & funcname)
	{
		if (!instance()->m_pDev) return false;
		if (instance()->log_state()==false)
			return true;
		instance()->m_mutex.lock();
		QTextStream st_out(instance()->m_pDev);
		st_out	<<	"\"" << subject <<"\"";
		st_out	<<	",\"" << detailed <<"\"";
		st_out	<<	",\"" << filename <<"\"";
		st_out	<<	",\"" << linenum <<"\"";
		st_out	<<	",\"" << funcname <<"\"";
		st_out	<<	",\"" << QThread::currentThreadId() <<"\"";
		st_out	<<	",\"" << QDateTime::currentDateTimeUtc().toString("yyyy-MM-ddTHH:mm:ss.zzz") <<"\"";
		st_out	<<	",\"" << clock() <<"\"\n";
		instance()->m_mutex.unlock();
		return true;
	}
private:
	QIODevice * m_pDev = nullptr;
	QString m_url;
	bool	m_bOwnDev = false;
	bool	m_bLogOn = true;
	QMutex  m_mutex;
};

#endif // PROFILE_LOG_H
