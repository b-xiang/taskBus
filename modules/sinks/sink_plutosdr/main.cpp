#include <QCoreApplication>
#include <QThread>
#include <QFile>
#include <QTextStream>
#include "listen_thread.h"
#include "cmdlineparser.h"
#include "tb_interface.h"
using namespace TASKBUS;
using namespace std;
//全局的终止标记
bool bfinished = false;
int do_iio(const cmdlineParser & args);

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);
	init_client();
#ifdef OFFLINEDEBUG
	FILE * old_stdin, *old_stdout;
	auto ars = debug("D:\\pid1008",&old_stdin,&old_stdout);
	const  cmdlineParser args (ars);
#else
	const cmdlineParser args (argc,argv);
#endif
	int ret = 0;
	QTextStream stmerr(stderr);
	//每个模块要响应 --information参数,打印自己的功能定义字符串。或者提供一个json文件。
	if (args.contains("information"))
	{
		QFile fp(":/sink_plutosdr."+QLocale::system().name()+".json");
		if (!fp.open(QIODevice::ReadOnly))
		{
			fp.setFileName(":/sink_plutosdr.json");
			fp.open(QIODevice::ReadOnly);
		}
		if (fp.isOpen())
		{
			QByteArray arr = fp.readAll();
			arr.push_back('\0');
			puts(arr.constData());
			fflush(stdout);
		}
		ret = -1;
	}
	else if (args.contains("function"))//正常运行模式
	{
		//用于接收消息的线程
		listen_thread * th = new listen_thread(args,&a);
		QObject::connect(th,&listen_thread::quit_app,&a,&QCoreApplication::quit);
		th->start();
		ret = do_iio(args);
	}
	else
	{
		stmerr<<"Error:Function does not exits.\n";
		ret = -1;
	}

	return ret;
}

