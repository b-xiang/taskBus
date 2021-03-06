#include "listen_thread.h"
#include "tb_interface.h"
listen_thread::listen_thread(
		const TASKBUS::cmdlineParser * cmdline,
		QObject * parent)
	:QThread(parent)
	,m_cmd(cmdline)
{

}

QAtomicInteger<quint64> listen_thread::m_refTms = 0;

void listen_thread::run()
{
	using namespace TASKBUS;
	bool bfinished = false;
	int iref_tms = 0;
	if (m_cmd)
		iref_tms = m_cmd->toInt("timestamp_in",0);

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
				if (strstr((const char *)packagedta.data(),"\"quit\":")!=nullptr)
				{
					fprintf(stderr,"Recived Quit Command.");
					fflush(stderr);
					bfinished = true;
				}
			}
			else if (header.subject_id==iref_tms)
			{
				if (header.data_length>=sizeof(quint64))
				{
					quint64 * tsp = (quint64 *) packagedta.data();
					m_refTms = *tsp;
				}
				else
					++m_refTms;
			}
		}
	}
	emit quit_app();
	return ;
}
