#include "listen_thread.h"
#include "tb_interface.h"
#include <QByteArray>
#include <QCoreApplication>
#include <QDebug>
#include <QFile>
reciv_thread::reciv_thread(QObject *parent)
	:QThread(parent)
{

}
/*!
 * \brief reciv_thread::run recieve package from stdin
 */
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
			bfinished = true;
			continue;
		}
		if ( is_control_subject(header))
		{
			//收到命令进程退出的广播消息,退出
			//quit command Recieved
			if (strstr(control_subject(header,packagedta).c_str(),"function=quit;")!=nullptr)
			{
				bfinished = true;
				qDebug()<<"Quit!";
				emit sig_quit();
			}
		}
		else
		{
			QByteArray arrPackage ((char *)packagedta.data(),packagedta.size());
			QByteArray arrHead((char *) &header,sizeof(subject_package_header));
			arrHead.append(arrPackage);
			emit new_package(arrHead);
		}
	}
	return ;
}

send_object::send_object(QObject *parent)
	:QObject(parent)
{

}

void send_object::send_package(QByteArray arr)
{
	using namespace TASKBUS;
	push_subject((const unsigned char *)arr.constData(),arr.size());
}
