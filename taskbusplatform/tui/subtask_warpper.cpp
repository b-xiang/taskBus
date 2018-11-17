/*! 基于封装的概念，一个工程可以作为一个整体，封装为一个模块。这个模块本身就是一系
 * 列的原模块（exe）连接而成。所有悬空的管脚都会被暴露出来。管脚的标号是局部的，
 * 所以模块外部的接口编号与模块内部是不冲突的。通路的标号却是全局的，因为通路代表了
 * 一组数据的完整性。这个EXE将被手工改为模块工程名一样的exe，比如模块为 sample.tbj,
 * 则EXE改为sample.exe，同时，需要涉及到的模块EXE列表另存为 sample.text 即可。
 *Based on the concept of encapsulation, a project can be packaged as a whole
 *  as a module. The module itself is a series of original modules (EXE)
 * connected. All dangling pins will be exposed.
 * The label of the pin is local, so the interface number outside the
 * module does not conflict with the inside of the module.
 * The path label is global, because the path represents the integrity of a
 * set of data. This exe will be manually changed to the same module project
 * name EXE, such as the module for SAMPLE.TBJ, then EXE to Sample.exe,
 * at the same time, need to refer to the module EXE list Save as Sample.text.
  */

#include <QByteArray>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>
#include <QProcess>
#include <QAtomicInt>
#include <stdio.h>
#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#endif
#include "core/taskcell.h"
#include "core/taskproject.h"
#include "cmdlineparser.h"
#include "tb_interface.h"
#include "listen_thread.h"
QAtomicInt  g_totalrev (0), g_totalsent (0);
//读取模块 Read Module
void load_modules(QStringList newfms, taskCell * cell);
void load_default_modules(QString filename, taskCell * cell);
//显示JSON Show JSON
QJsonObject output_json(QString name,taskProject * proj);
using namespace TASKBUS;
int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);

#ifdef WIN32
	setmode(fileno(stdout),O_BINARY);
	setmode(fileno(stdin),O_BINARY);
#endif

	//接收线程 Receive thread
	reciv_thread * th_reciv = new reciv_thread(&a);
	send_object * send_obj = new send_object(&a);

	//加载工程文件 Loading project files
	QFileInfo info(QCoreApplication::applicationFilePath());
	QString strModFileName = QCoreApplication::applicationDirPath()+"/"+ info.completeBaseName()+".tbj";
	QString strHistoryModFileName = QCoreApplication::applicationDirPath()+"/"+ info.completeBaseName()+".text";
	//1.预先读入模块 1. Pre-read module
	taskCell cell_all;
	load_default_modules(strHistoryModFileName,&cell_all);
	QCoreApplication::processEvents();

	//2.加载工程 2. Loading the project
	taskProject * prj = new taskProject(&a);
	QFile fo(strModFileName);
	if (fo.open(QIODevice::ReadOnly))
	{
		QByteArray ar = fo.readAll();
		prj->fromJson(ar,&cell_all);
		prj->setWarpperPrj(true);
		prj->refresh_idxes();
		fo.close();
	}
	QCoreApplication::processEvents();

	//3.解释命令行参数 3. Interpreting command-line arguments
	const cmdlineParser args (argc,argv);
	int ret = 0;

	if (args.contains("information"))
	{
		QTextStream ste(stdout,QIODevice::WriteOnly);
		QJsonObject obj = output_json(info.completeBaseName(),prj);
		QJsonDocument doc(obj);
		ste<<QString::fromStdString(doc.toJson().toStdString().c_str());
		ret = -1;
	}
	else if (args.contains("help",info.completeBaseName().toStdString()))
	{
		QJsonObject obj = output_json(info.completeBaseName(),prj);
		QJsonDocument doc(obj);
		puts(doc.toJson().toStdString().c_str());
		fprintf(stderr,"\nno function valid in func list.\n");
		ret = -1;
	}
	else
	{
		//提取各个引脚的ID Extract the IDs of each pin
		QStringList lst_hang_ins = prj->idxout_hang_fullname2in().keys();
		foreach(QString instr,lst_hang_ins)
		{
			if (args.contains(instr.toStdString()))
				prj->set_outside_id_in(instr, args.toInt(instr.toStdString(),0));
			else if (args.contains(instr.toUtf8().toStdString()))
				prj->set_outside_id_in(instr, args.toInt(instr.toUtf8().toStdString(),0));
			else if (args.contains(instr.toLocal8Bit().toStdString()))
				prj->set_outside_id_in(instr, args.toInt(instr.toLocal8Bit().toStdString(),0));

		}
		QStringList lst_hang_outs = prj->idxout_hang_fullname2out().keys();
		foreach(QString outstr,lst_hang_outs)
		{
			if (args.contains(outstr.toStdString()))
				prj->set_outside_id_out(outstr, args.toInt(outstr.toStdString(),0));
			else if (args.contains(outstr.toUtf8().toStdString()))
				prj->set_outside_id_out(outstr, args.toInt(outstr.toUtf8().toStdString(),0));
			else if (args.contains(outstr.toLocal8Bit().toStdString()))
				prj->set_outside_id_out(outstr, args.toInt(outstr.toLocal8Bit().toStdString(),0));
		}

		//运行工程 run project
		a.connect (prj,&taskProject::sig_outside_new_package,send_obj, &send_object::send_package,Qt::QueuedConnection);
		a.connect (th_reciv, &reciv_thread::new_package,prj,&taskProject::slot_outside_recieved,Qt::QueuedConnection);
		a.connect (th_reciv, &reciv_thread::sig_quit,prj,
				   static_cast<void (taskProject::*)()>(&taskProject::stop_project),Qt::QueuedConnection);
		a.connect (prj, &taskProject::sig_stopped,&a,&QCoreApplication::quit,Qt::QueuedConnection);
		th_reciv->start();
		prj->start_project();
		ret = a.exec();
		prj->stop_project();

	}

	QCoreApplication::processEvents();

	a.quit();
	return ret;
}

/*!
 * \brief load_default_modules 从默认加载脚本加载模块
 * load default modules from text script file
 * \param filename 脚本文件名，默认为"default.text" script file name.
 * \param cell 导入cell  import to cell
 */
void load_default_modules(QString filename, taskCell * cell)
{
	QFile fin(filename);
	if (fin.open(QIODevice::ReadOnly)==false)
		return;
	QTextStream st(&fin);
	QStringList lstNames;
	while (st.atEnd()==false)
	{
		lstNames<< st.readLine();
	}
	fin.close();
	load_modules(lstNames,cell);

}

void load_modules(QStringList newfms, taskCell * cell)
{
	//QJSon
	foreach (QString newfm, newfms)
	{
		//首先试图找JSON文件.
		//Step 1, try to load from json
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
		else//找不到JSON，则调用命令行
			//else , from cmdline
		{
			QProcess proc;
			proc.start(newfm,QStringList()<<"--information");
			proc.waitForFinished(10000);
			array.append(proc.readAll());
			proc.kill();
		}
		if (array.size())
		{
			if (false==cell->initFromJson(array,newfm))
			{
				QString str = QString::fromLocal8Bit(array.data());
				QByteArray bt = QByteArray::fromStdString(str.toStdString());
				cell->initFromJson(bt,newfm);
			}
		}

	}
}
QJsonObject output_json(QString name,taskProject * proj)
{
	QJsonObject obj_root;
	QJsonObject obj_func;
	obj_func["name"] = name;
	obj_func["parameters"] = QJsonObject();
	QJsonObject obj_insubs;
	QStringList lst_hang_ins = proj->idxout_hang_fullname2in().keys();
	foreach(QString instr,lst_hang_ins)
	{
		unsigned int in_id = proj->idxout_hang_fullname2in()[instr];
		unsigned int instance = proj->idxout_hang_in2instance()[in_id];
		const taskCell * cell = proj->vec_cells()[proj->idx_instance2vec()[instance]];
		const QString inname = proj->idxout_hang_in2name()[in_id];
		const QString tooltip = cell->in_subject_tooltip(cell->function_names().first(),inname);
		const QString type = cell->in_subject_item(cell->function_names().first(),inname,"type");
		QJsonObject obj_inter;
		obj_inter["type"] = type;
		obj_inter["tooltip"] = tooltip;
		obj_insubs[instr] = obj_inter;
	}
	obj_func["input_subjects"] = obj_insubs;


	QJsonObject obj_outsubs;
	QStringList lst_hang_outs = proj->idxout_hang_fullname2out().keys();
	foreach(QString outstr,lst_hang_outs)
	{
		unsigned int out_id = proj->idxout_hang_fullname2out()[outstr];
		unsigned int instance = proj->idxout_hang_out2instance()[out_id];
		const taskCell * cell = proj->vec_cells()[proj->idx_instance2vec()[instance]];
		const QString outname = proj->idxout_hang_out2name()[out_id];
		const QString tooltip = cell->out_subject_tooltip(cell->function_names().first(),outname);
		const QString type = cell->out_subject_item(cell->function_names().first(),outname,"type");
		QJsonObject obj_inter;
		obj_inter["type"] = type;
		obj_inter["tooltip"] = tooltip;
		obj_outsubs[outstr] = obj_inter;
	}
	obj_func["output_subjects"] = obj_outsubs;


	obj_root[name] = obj_func;

	return obj_root;

}
