#include "dialognetp2p.h"
#include <QApplication>
#include <QVector>
#include <QMap>
#include <QFile>
#include "cmdlineparser.h"
#include "tb_interface.h"
using namespace TASKBUS;
//#define OFFLINEDEBUG
int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	init_client();

#ifdef OFFLINEDEBUG
	FILE * old_stdin, *old_stdout;
	auto ars = debug("D:/log/pid1290",&old_stdin,&old_stdout);
	const  cmdlineParser args (ars);
#else
	const cmdlineParser args (argc,argv);
#endif


	int ret = 0;

	//每个模块要响应 --information参数,打印自己的功能定义字符串。或者提供一个json文件。
	if (args.contains("information"))
	{
		QFile fp(":/json/network_p2p.exe.json");
		if (fp.open(QIODevice::ReadOnly))
		{
			QByteArray arr = fp.readAll();
			arr.push_back('\0');
			puts(arr.constData());
			fflush(stdout);
		}
		ret = -1;
		return ret;
	}
	else
	{
		const QString para_address = QString::fromStdString(args.toString("address","127.0.0.1"));
		const int para_port = args.toInt("port",9527);
		const int para_mod = args.toInt("mod",0);
		const int para_hide = args.toInt("hide",0);
		const int instance = args.toInt("instance",0);
		QVector<int> paravec_outpts;
		QMap<int,int> paramap_inputs;
		for (int i = 0; i < 256;++i)
		{
			QString key = QString("in%1").arg(i);
			const int p_v = args.toInt(key.toStdString(),-1);
			if (p_v>0)
				paramap_inputs[p_v] = i;
			key = QString("out%1").arg(i);
			const int p_o = args.toInt(key.toStdString(),-1);
			if (p_o>0)
				paravec_outpts.push_back(p_o);
			else
				paravec_outpts.push_back(-1);
		}

		DialogNetP2P w(instance);
		if (!para_hide)
			w.show();

		w.setNetPara(para_address,para_port,para_mod);
		w.setMap(paramap_inputs,paravec_outpts);

		 if (args.contains("function"))
		 {
			 w.startWork();
		 }

		ret = a.exec();
	}
	return ret;
}
