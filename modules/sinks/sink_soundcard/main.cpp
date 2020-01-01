#include "dialogsoundcard.h"
#include <QApplication>
#include <QFile>
#include <QByteArray>
#include "cmdlineparser.h"
#include "tb_interface.h"

const bool TASKBUSDBG = false;

using namespace TASKBUS;
int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	init_client();
	cmdlineParser args;
	//Debug
	if (TASKBUSDBG)
	{
		auto cmdline = TASKBUS::debug("d:/log/pid2837",0,0);
		args.parser(cmdline);
	}
	else
		args.parser(argc,argv);

	int ret = 0;
	//Arg--information参数,打印自己的功能定义字符串。
	if (args.contains("information"))
	{
		QFile fp(":/json/sink_soundcard.json");
		if (fp.open(QIODevice::ReadOnly))
		{
			QByteArray arr = fp.readAll();
			arr.push_back('\0');
			puts(arr.constData());
			fflush(stdout);
		}
	}
	else
	{
		DialogSoundCard w(&args);
		int hiden = args.toInt("hide",0);
		if (hiden==0)
			w.show();
		//用于接收消息的线程
		ret = a.exec();
	}
	return ret;
}
