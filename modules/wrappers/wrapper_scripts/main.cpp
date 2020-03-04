#include "dlgwrpscript.h"
#include <QApplication>
#include "tb_interface.h"
#include "cmdlineparser.h"
using namespace TASKBUS;
const int OFFLINEDEBUG = 0;
int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	TASKBUS::init_client();
	DlgWrpScript w;
	//解释命令行
	cmdlineParser args;
	if (OFFLINEDEBUG==0)
	{
		args.parser(argc,argv);
		for (int i=1;i<argc;++i)
			w.m_lstArgs << argv[i];
	}
	else
	{
		FILE * old_stdin, *old_stdout;
		auto ars = debug("D:/log/pid1290",&old_stdin,&old_stdout);
		args.parser(ars);
		for (size_t i=1;i<ars.size();++i)
			w.m_lstArgs << ars[i].c_str();
	}


	if (args.contains("function")||
			args.contains("information"))
		w.run();
	else
		w.show();

	return a.exec();
}
