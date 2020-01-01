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
	unsigned int iref_tms = 0;
	unsigned int iref_wav = 0;
	unsigned int iref_chn = 0;
	if (m_cmd)
	{
		iref_tms = m_cmd->toUInt("timestamp_in",0);
		iref_wav = m_cmd->toUInt("wav",0);
		iref_chn = m_cmd->toUInt("channel",0);
	}

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
		if (!packagedta.empty())
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
			else if (header.subject_id==iref_wav && iref_chn>0)
			{
				QByteArray arr((char * ) packagedta.data(),packagedta.size());
				emit sig_play(arr);

			}
		}
	}
	emit quit_app();
}
