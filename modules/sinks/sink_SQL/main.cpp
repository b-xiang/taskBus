#include "dialogsql.h"
#include <QApplication>
#include <QTextStream>
#include "cmdlineparser.h"
#include "tb_interface.h"
#include <QSqlDatabase>
#include <QFile>
using namespace TASKBUS;
//#define OFFLINEDEBUG

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	init_client();

#ifdef OFFLINEDEBUG
	FILE * old_stdin, *old_stdout;
	auto ars = debug("D:\\Dynamic\\GrayEagle\\debug\\pid2548",&old_stdin,&old_stdout);
	const  cmdlineParser args (ars);
#else
	const cmdlineParser args (argc,argv);
#endif


	int ret = 0;

	//每个模块要响应 --information参数,打印自己的功能定义字符串。或者提供一个json文件。
	if (args.contains("information"))
	{
		QFile fp(":/json/sink_SQL.exe.json");
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
		QTextStream st(stderr);
		st<<"Database Avalibale:";
		QStringList lst = QSqlDatabase::drivers();
		foreach (QString l,lst)
			st<<l<<",";
		st.flush();

		DialogSQL w(&args);
		w.show();
		if (args.toInt("hide_window",0)!=0)
			w.hide();
		ret = a.exec();
	}
	return ret;
}
