#include "listen_thread.h"
#include "tb_interface.h"
#include <QByteArray>
#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#endif
reciv_thread::reciv_thread(QObject *parent)
	:QThread(parent)
{

}

void reciv_thread::run()
{
	bool bfinished = false;
	using namespace TASKBUS;
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
			if (strstr(control_subject(header,packagedta).c_str(),"function=quit;"))
			{
				bfinished = true;
				qDebug()<<"Quit!";
				emit sig_quit();
			}
		}
		else
		{
			QByteArray arrHead((char *) &header,sizeof(subject_package_header));
			if (packagedta.size())
			{
				QByteArray arrPackage ((char *)packagedta.data(),packagedta.size());
				arrHead.append(arrPackage);
			}
			emit new_package(arrHead);
		}
	}
	return ;
}

