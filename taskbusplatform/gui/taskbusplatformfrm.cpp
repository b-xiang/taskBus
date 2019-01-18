#include "taskbusplatformfrm.h"
#include "ui_taskbusplatformfrm.h"
#include <QCloseEvent>
#include <QDir>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QMdiSubWindow>
#include <QSettings>
#include <QDateTime>
#include "pdesignerview.h"
#include "watchdog/tbwatchdog.h"
#include "dlgabout.h"

int taskBusPlatformFrm::m_doc_ins = 0;

taskBusPlatformFrm::taskBusPlatformFrm(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::taskBus),
	m_pMsgModel(new QStandardItemModel(this)),
	m_pClassModel(new QStandardItemModel(this)),
	m_pTrayIcon(new QSystemTrayIcon(this))
{
	ui->setupUi(this);	
	setCentralWidget(ui->mdiArea);
	//全部模块 Create Module for All taskModules
	m_pRefModule = m_toolModules[tr("All")] = new taskModule(true,this);
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
	tabifyDockWidget(ui->dockWidget_message,ui->dockWidget_watch);
	tabifyDockWidget(ui->dockWidget_message,ui->dockWidget_props);

	m_iconTray[0].addFile(":/taskBus/images/ticon1.png");
	m_iconTray[1].addFile(":/taskBus/images/ticon2.png");
	m_pTrayIcon->setIcon(m_iconTray[0]);
	m_pTrayIcon->show();

	QMenu * me = new QMenu(this);
	me->addAction(ui->actionhideWindow);
	me->addAction(ui->action_Exit);
	ui->menu_View->insertAction(ui->actionhideWindow,ui->dockWidget_message->toggleViewAction());
	ui->menu_View->insertAction(ui->actionhideWindow,ui->dockWidget_modules->toggleViewAction());
	ui->menu_View->insertAction(ui->actionhideWindow,ui->dockWidget_props->toggleViewAction());
	ui->menu_View->insertAction(ui->actionhideWindow,ui->dockWidget_watch->toggleViewAction());
	m_pTrayIcon->setContextMenu(me);
	connect(m_pTrayIcon,&QSystemTrayIcon::activated,this,&taskBusPlatformFrm::slot_traymessage);

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

void taskBusPlatformFrm::slot_traymessage(QSystemTrayIcon::ActivationReason r)
{
	switch (r)
	{
	case QSystemTrayIcon::DoubleClick:
		ui->actionhideWindow->setChecked(false);
		break;
	default:
		break;
	}
}

void taskBusPlatformFrm::timerEvent(QTimerEvent *event)
{
	static int pp = 0;
	if (m_nTmid==event->timerId())
	{
		extern QAtomicInt  g_totalrev, g_totalsent;
		QString s = QString().sprintf("down %.2lf Mbps, up %.2lf Mbps",
									  g_totalrev * 8.0 / 1024 / 1024,
									  g_totalsent * 8.0 / 1024 / 1024
									  );
		m_pStatus->setText(s);
		if (g_totalrev>0 || g_totalrev>0)
		{
			++pp;
			m_pTrayIcon->setIcon(m_iconTray[pp%2]);
		}
		else if (pp)
		{
			pp = 0;
			m_pTrayIcon->setIcon(m_iconTray[0]);
		}
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
	else
		ui->treeView_props->setModel(0);
}

void taskBusPlatformFrm::on_action_About_triggered()
{
	DlgAbout about(this);
	if (about.exec()==QDialog::Accepted)
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
	m_pTrayIcon->showMessage(tr("Init Modules..."),tr("Init modules from default_mods.text"),QSystemTrayIcon::Information, 1000);
	load_modules(lstNames);
	m_pTrayIcon->showMessage(tr("Succeed."),tr("Init modules from default_mods.text succeed!"),QSystemTrayIcon::Information, 2000);
	emit hideSplash();
}
void taskBusPlatformFrm::save_default_modules()
{
	const QSet<QString> & s = refModule()->full_paths();

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

void taskBusPlatformFrm::closeEvent(QCloseEvent * event)
{
	bool bModified = false;
	bool bRunning = false;
	QStringList lsN = m_activePagesFileName.keys();
	foreach (QString k, lsN)
	{
		QWidget * w = m_activePagesFileName[k]->widget();
		if (w)
		{
			PDesignerView * v = qobject_cast<PDesignerView*>(w);
			if(v)
			{
				bModified = bModified || v->modified();
				bRunning = bRunning || v->project()->isRunning();
			}
		}
	}
	if (bRunning)
	{
		QMessageBox::information(this,tr("Still running"),tr("Project is still running, please stop all projects first."),QMessageBox::Ok);
		event->ignore();
		return;
	}
	if (bModified)
	{
		if (QMessageBox::information(this,tr("Close without saving?"),tr("Project has been modified, Close it anyway?"),
									 QMessageBox::Ok,
									 QMessageBox::Cancel)!=QMessageBox::Ok)
		{
			event->ignore();
			return;
		}
	}
	event->accept();
}

void taskBusPlatformFrm::on_actionhideWindow_toggled(bool arg1)
{
	if (arg1)
		hide();
	else
		show();
}



void taskBusPlatformFrm::on_listView_modules_doubleClicked(const QModelIndex &index)
{
	QMdiSubWindow * sub = ui->mdiArea->activeSubWindow();
	if (sub)
	{
		PDesignerView * dv = qobject_cast<PDesignerView *>(sub->widget());
		if (dv)
		{
			if (ui->listView_modules->model())
			{
				QModelIndexList lst;
				lst<<index;
				QMimeData * d = ui->listView_modules->model()->mimeData(lst);
				if (d)
					dv->addCell(d);
			}

		}
	}
}
