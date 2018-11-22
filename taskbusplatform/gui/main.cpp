#include "taskbusplatformfrm.h"
#include <QApplication>
#include <QLibraryInfo>
#include <QTranslator>
#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#include <windows.h>
#endif
#include <QAtomicInt>
#include <QProcess>
#include <QSettings>
#include <QDir>
#include "watchdog/tbwatchdog.h"
//全局吞吐量 global IO speed recorder
QAtomicInt  g_totalrev (0), g_totalsent (0);

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	tb_watch_dog()->watch();
	QDir dir("/");
	dir.setCurrent(app.applicationDirPath());

	QTranslator qtTranslator;
	qtTranslator.load("qt_" + QLocale::system().name(),
					  QLibraryInfo::location(QLibraryInfo::TranslationsPath));
	app.installTranslator(&qtTranslator);

	QTranslator appTranslator;
	QString strTransLocalFile =
			QCoreApplication::applicationDirPath()+"/" +
			QCoreApplication::applicationName()+"_"+
			QLocale::system().name()+".qm";
	appTranslator.load(strTransLocalFile );
	app.installTranslator(&appTranslator);
	taskBusPlatformFrm w;
	w.show();
#ifdef WIN32
	setmode(fileno(stdout),O_BINARY);
	setmode(fileno(stdin),O_BINARY);
#endif



	return app.exec();
}
