#include "listen_thread.h"
#include "tb_interface.h"
listen_thread::listen_thread(QObject * parent)
	:QThread(parent)
{

}
void listen_thread::run()
{
	using namespace TASKBUS;
	bool bfinished = false;
	int rectg = 0;
	while (false==bfinished && rectg<16)
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
		}
		QByteArray arr;
		arr.append((char *)&header,sizeof(subject_package_header));
		arr.append((char *)packagedta.data(),packagedta.size());
		emit new_package(arr);
		if (bfinished)
		{
			msleep(100);
			++rectg;
		}
	}
	emit quit_app();
	return ;
}
