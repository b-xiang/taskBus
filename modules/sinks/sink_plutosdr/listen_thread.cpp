#include "listen_thread.h"
#include "tb_interface.h"
#include <QMutex>
extern QVector<short> vec_global_buf;
extern QMutex mutex_buf;

listen_thread::listen_thread(const TASKBUS::cmdlineParser & a,QObject * parent)
	:QThread(parent)
	, args(a)
{

}
void listen_thread::run()
{
	using namespace TASKBUS;
	const int instance	  = args.toInt("instance",0);
	const int idestin	  = args.toInt("destin",0);
	const int timestamp	  = args.toInt("timestamp",0);
	bool bfinished = false;
	while (false==bfinished)
	{
		subject_package_header header;
		std::vector<unsigned char> packagedta = pull_subject(&header);
		if (!is_valid_header(header))
		{
			fprintf(stderr,"Recived BAD Command.");
			fflush(stderr);
			msleep(100);
			continue;
		}
		if (packagedta.size())
		{
			if ( is_control_subject(header))
			{
				//收到命令进程退出的广播消息,退出
				if (strstr((const char *)packagedta.data(),"function=quit;")!=nullptr)
				{
					fprintf(stderr,"Recived Quit Command.");
					fflush(stderr);
					bfinished = true;
				}
			}
			else if (header.subject_id==idestin)
			{
				int iqpaires = packagedta.size()/4*2;
				short * iq = (short *)(packagedta.data());
				mutex_buf.lock();
				std::copy(iq,iq+iqpaires,std::back_inserter(vec_global_buf));
				mutex_buf.unlock();
			}
		}
	}
	emit quit_app();
	return ;
}
