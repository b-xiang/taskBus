#include "taskbusplatformfrm.h"
#include "ui_taskbusplatformfrm.h"
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QMdiSubWindow>
#include <QSettings>
#include "pdesignerview.h"

int taskBusPlatformFrm::m_doc_ins = 0;

taskBusPlatformFrm::taskBusPlatformFrm(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::taskBus),
	m_pMsgModel(new QStandardItemModel(this)),
	m_pClassModel(new QStandardItemModel(this))
{
	ui->setupUi(this);
	setCentralWidget(ui->mdiArea);
	//全部模块 Create Module for All taskModules
	m_toolModules[tr("All")] = new taskModule(true,this);
	ui->listView_modules->setModel(m_toolModules[tr("All")]);

	//类别 ComboBox for classes
	m_pClassModel->appendRow(new QStandardItem(tr("All")));
	ui->comboBox_class->setModel(m_pClassModel);
	//消息 message module
	ui->listView_messages->setModel(m_pMsgModel);
	//状态栏 status bar
	m_pStatus = new QLabel(this);
	statusBar()->addWidget(m_pStatus);

	m_nTmid = startTimer(1000);
	tabifyDockWidget(ui->dockWidget_message,ui->dockWidget_props);


	//加载模块 load default modules
	load_default_modules();
}

taskBusPlatformFrm::~taskBusPlatformFrm()
{
	QList<QMdiSubWindow *> lst = ui->mdiArea->subWindowList();
	foreach (QMdiSubWindow * sub, lst)
	{
		PDesignerView * dv = qobject_cast<PDesignerView *>(sub->widget());
		if (dv)
			dv->stop();
	}
	delete ui;
}
void taskBusPlatformFrm::timerEvent(QTimerEvent *event)
{
	if (m_nTmid==event->timerId())
	{
		extern QAtomicInt  g_totalrev, g_totalsent;
		QString s = QString().sprintf("down %.2lf Mbps, up %.2lf Mbps",
									  g_totalrev * 8.0 / 1024 / 1024,
									  g_totalsent * 8.0 / 1024 / 1024
									  );
		m_pStatus->setText(s);
		g_totalsent = 0;
		g_totalrev = 0;

	}
}

QString taskBusPlatformFrm::inifile()
{
	return QCoreApplication::applicationFilePath()+".ini";
}

void taskBusPlatformFrm::slot_showPropModel(QObject * objModel)
{
	taskModule * pm = qobject_cast<taskModule *>(objModel);
	if (pm)
	{
		ui->treeView_props->setModel(pm);
		ui->treeView_props->collapseAll();
		ui->treeView_props->expand(pm->index(0,0));
		ui->treeView_props->expand(pm->paraIndex());
		ui->treeView_props->scrollToBottom();
		//ui->treeView_props->expandAll();
	}
}

void taskBusPlatformFrm::on_action_About_triggered()
{
	QMessageBox::about(this,tr("taskBus"),tr("Used to orgnize process-based modules. by goldenhawking, 2018 for opensource usage."));
	QApplication::aboutQt();
}

void taskBusPlatformFrm::on_action_Start_project_triggered()
{
	QMdiSubWindow * sub = ui->mdiArea->activeSubWindow();
	if (sub)
	{
		PDesignerView * dv = qobject_cast<PDesignerView *>(sub->widget());
		if (dv)
		{
			dv->run();
			on_mdiArea_subWindowActivated(sub);
		}
	}
}


void taskBusPlatformFrm::on_action_stop_project_triggered()
{
	QMdiSubWindow * sub = ui->mdiArea->activeSubWindow();
	if (sub)
	{
		PDesignerView * dv = qobject_cast<PDesignerView *>(sub->widget());
		if (dv)
		{
			dv->stop();
			on_mdiArea_subWindowActivated(sub);
		}
	}

}

void taskBusPlatformFrm::load_default_modules()
{
	QString DefaultFile = QCoreApplication::applicationDirPath() + "/default_mods.text";
	QFile fin(DefaultFile);
	if (fin.open(QIODevice::ReadOnly)==false)
		return;
	QTextStream st(&fin);
	QStringList lstNames;
	while (st.atEnd()==false)
	{
		lstNames<< st.readLine();
	}
	fin.close();
	load_modules(lstNames);

}
void taskBusPlatformFrm::save_default_modules()
{
	const QSet<QString> & s = m_toolModules[tr("All")]->full_paths();

	QString DefaultFile = QCoreApplication::applicationDirPath() + "/default_mods.text";
	QFile fin(DefaultFile);
	if (fin.open(QIODevice::WriteOnly)==false)
		return;
	QTextStream st(&fin);
	QDir dir(QCoreApplication::applicationDirPath());
	foreach (QString sf, s) {
		QString strRelPath;
		if (QDir::isAbsolutePath(sf)==true)
			strRelPath  =  dir.relativeFilePath(sf);
		else
			strRelPath = sf;
		if (strRelPath.length()>0 && strRelPath.length()<sf.length())
			st<<strRelPath<<"\n";
		else
			st<<sf<<"\n";
	}

	fin.close();
}


void taskBusPlatformFrm::on_mdiArea_subWindowActivated(QMdiSubWindow *arg1)
{
	if (!arg1)
	{
		ui->action_Start_project->setEnabled(false);
		ui->action_stop_project->setEnabled(false);
		ui->action_Save_Project->setEnabled(false);
		ui->dockWidget_props->setEnabled(false);
		ui->treeView_props->setModel(0);
		return;
	}
	else
	{
		ui->action_Save_Project->setEnabled(true);
	}
	PDesignerView * dv = qobject_cast<PDesignerView *>(arg1->widget());
	if (dv)
	{
		int noden = dv->selectedNode();
		if (noden<0||noden>=dv->project()->vec_cells().size())
			ui->treeView_props->setModel(0);
		else
		{
			taskCell * c = dv->project()->vec_cells()[noden];
			taskModule * m = dynamic_cast<taskModule *>(c);
			if (m!=ui->treeView_props->model() && m)
			{
				ui->treeView_props->setModel(m);
				ui->treeView_props->collapseAll();
				ui->treeView_props->expand(m->index(0,0));
				ui->treeView_props->expand(m->paraIndex());
				ui->treeView_props->scrollToBottom();
			}
			else if (!m)
				ui->treeView_props->setModel(0);
		}

		ui->action_Start_project->setEnabled(!dv->is_running());
		ui->action_stop_project->setEnabled(dv->is_running());
		ui->dockWidget_props->setEnabled(!dv->is_running());
		dv->setEnabled(!dv->is_running());
	}
}

void taskBusPlatformFrm::on_comboBox_class_currentIndexChanged(int index)
{
	if (index>=0 && index<m_pClassModel->rowCount())
	{
		QString name = m_pClassModel->data(m_pClassModel->index(index,0)).toString();
		if (m_toolModules.contains(name))
			ui->listView_modules->setModel(m_toolModules[name]);
	}
}
