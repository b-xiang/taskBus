#include "taskbusplatformfrm.h"
#include "ui_taskbusplatformfrm.h"
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QProcess>
#include <QSettings>
#include <QThread>
#include <QDateTime>
#include <QMap>
void taskBusPlatformFrm::load_modules(QStringList newfms)
{
	//QJSon
	struct mod_info{
		QString newfm;
		QString className;
		QByteArray bt;
	};

	QMap<QString, mod_info> minfo;
	foreach (QString newfm, newfms)
	{
		emit showSplash(tr("Loading ")+newfm,Qt::AlignBottom,QColor(0,0,0));
		//首先试图找JSON文件.
		//We can provide a JSON file along with exe file.This approach
		//will significantly boost loading approach.
		QFileInfo infojs(newfm);
		QString abPath = infojs.absolutePath();
		QString abBName = infojs.completeBaseName();
		//Try JSON
		QString jsonfm = newfm+".json";
		QFile fjson(jsonfm), fjsonb(abPath+"/"+abBName+".json");
		QByteArray array ;
		if (fjson.open(QIODevice::ReadOnly))
		{
			array = fjson.readAll();
			fjson.close();
		}
		else if (fjsonb.open(QIODevice::ReadOnly))
		{
			array = fjsonb.readAll();
			fjsonb.close();
		}
		//找不到JSON，则调用命令行
		//Or, You can respond to the "--information" parameter and then
		//output the JSON information.
		else
		{
			QProcess proc;
			proc.start(newfm,QStringList()<<"--information");
			array.append(proc.readAll());
			proc.waitForFinished(10000);
			array.append(proc.readAll());
			proc.kill();
		}
		int idx = array.indexOf('{');
		if (idx>0)
			array = array.mid(idx);
		idx = array.lastIndexOf("}");
		if (idx>0)
			array = array.left(idx+1);

		if (array.size())
		{
			taskModule mod;
			QByteArray bt = array;
			bool bOk = mod.initFromJson(bt,newfm);
			if (!bOk)
			{
				QString str = QString::fromLocal8Bit(array.data());
				bt = QByteArray::fromStdString(str.toStdString());
				bOk=mod.initFromJson(bt,newfm);
			}
			if (bOk)
			{
				mod_info info;
				QString className = mod.function_class(mod.function_firstname());
				info.bt = bt;
				info.newfm = newfm;
				info.className = className.trimmed();
				minfo[className+"|" + taskCell::pureName(mod.function_firstname())] = info;
			}
		}
		QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
	}

	QStringList keys = minfo.keys();
	foreach (QString k, keys)
	{
		QByteArray bt = minfo[k].bt;
		QString newfm = minfo[k].newfm;
		QString className = minfo[k].className;
		m_toolModules[tr("All")]->initFromJson(bt,newfm);
		if (m_toolModules.contains(className)==false)
		{
			m_toolModules[className] = new taskModule(true,this);
			m_pClassModel->appendRow(new QStandardItem(className));
		}
		m_toolModules[className]->initFromJson(bt,newfm);

	}
}

/*!
 * \brief taskBusPlatformFrm::on_action_Load_Module_triggered
 * 用户选择一个EXE文件并导入 User choose a exe file to add into module list.
 */
void taskBusPlatformFrm::on_action_Load_Module_triggered()
{
	QSettings settings(inifile(),QSettings::IniFormat);
	QString strLastModuleDir = settings.value("history/strLastModuleDir","./").toString();
#ifdef WIN32
	QStringList newfms = QFileDialog::getOpenFileNames(this,tr("load modules"),strLastModuleDir,
								 "exe files (*.exe);;All files(*.*)"
								 );
#else
	QStringList newfms = QFileDialog::getOpenFileNames(this,tr("load modules"),strLastModuleDir,
								 "exe files (* *.exe);;All files(*.*)"
								 );
#endif

	load_modules(newfms);
	if (newfms.size()>0)
	{
		QFileInfo info(newfms[0]);
		settings.setValue("history/strLastModuleDir",info.absolutePath());
	}
	save_default_modules();
}

void taskBusPlatformFrm::slot_showMsg(QString str)
{
	QString prefix = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
	m_pMsgModel->appendRow(new QStandardItem(prefix+str));
	if (m_pMsgModel->rowCount()>128)
		m_pMsgModel->removeRows(0,m_pMsgModel->rowCount()-128);
	ui->listView_messages->scrollToBottom();
}
