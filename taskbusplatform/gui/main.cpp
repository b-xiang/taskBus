﻿#include "taskbusplatformfrm.h"
#include <QApplication>
#include <QDebug>
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
#include <QSplashScreen>
#include <QPixmap>
#include <QDir>
#include "watchdog/tbwatchdog.h"
#include "watchdog/profile_log.h"
//全局吞吐量 global IO speed recorder
QAtomicInt  g_totalrev (0), g_totalsent (0);
void create_custom_item_editor();
int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	//init ProfileLog
	profile_log::init();
	//If you want to do profile test, please turn this on (true)
	profile_log::set_log_state(true);
	LOG_PROFILE("Program","Main Start.");
	//Init watchdog
	tb_watch_dog().watch();
	//Change CurrentDir
	QDir dir("/");
	dir.setCurrent(app.applicationDirPath());

	//Install translators
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
	//using custon item editor, for double int values.
	create_custom_item_editor();
	//show splahs screen
	QSplashScreen screen(QPixmap(":/taskBus/images/taskBus.png"));
	screen.show();
	app.processEvents(QEventLoop::ExcludeUserInputEvents);

	//init main frame
	taskBusPlatformFrm w;
	QObject::connect(&w,&taskBusPlatformFrm::showSplash,
					 &screen, &QSplashScreen::showMessage);
	QObject::connect(&w,&taskBusPlatformFrm::hideSplash,
					 &screen, &QSplashScreen::hide);
	app.processEvents(QEventLoop::ExcludeUserInputEvents);
	//Load default modules
	w.load_default_modules();
	w.show();

#ifdef WIN32
	setmode(fileno(stdout),O_BINARY);
	setmode(fileno(stdin),O_BINARY);
#endif

	g_totalrev = 0;
	g_totalsent = 0;

	LOG_PROFILE("Program","Main Inited.");
	if (profile_log::log_state())
		qDebug() << "Log File:"<<profile_log::url();

	int ret = app.exec();
	LOG_PROFILE("Program",QString("Main End with return value %1.").arg(ret));

	return ret;
}
