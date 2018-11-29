#include "taskbusplatformfrm.h"
#include "ui_taskbusplatformfrm.h"
#include <QDebug>
#include <QMdiSubWindow>
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QSettings>
#include <QJsonObject>
#include <QJsonDocument>
#include <QThread>
#include "pdesignerview.h"

void taskBusPlatformFrm::on_action_New_Project_triggered()
{
	PDesignerView * v = new PDesignerView();
	QString fm = QString("Untitled %1").arg(++m_doc_ins);
	v->setWindowTitle(fm);
	v->setFullFileName(fm);
	QMdiSubWindow * wnd = ui->mdiArea->addSubWindow(v);
	m_activePagesFileName[fm] = wnd;
	v->show();
	connect (v,&PDesignerView::sig_showProp,this,&taskBusPlatformFrm::slot_showPropModel,Qt::QueuedConnection);
	connect (v,&PDesignerView::sig_message,this,&taskBusPlatformFrm::slot_showMsg,Qt::QueuedConnection);
	connect (v,&PDesignerView::sig_openprj,this,&taskBusPlatformFrm::slot_openprj,Qt::QueuedConnection);
	connect (v,&PDesignerView::sig_projstarted,this,&taskBusPlatformFrm::slot_projstarted,Qt::QueuedConnection);
	connect (v,&PDesignerView::sig_projstopped,this,&taskBusPlatformFrm::slot_projstopped,Qt::QueuedConnection);

}
void taskBusPlatformFrm::on_action_Save_Project_triggered()
{
	QMdiSubWindow * sub = ui->mdiArea->activeSubWindow();
	if (sub)
	{
		PDesignerView * dv = qobject_cast<PDesignerView *>(sub->widget());
		QString oldfm = dv->fullFileName();
		if (dv)
		{
			QSettings settings(inifile(),QSettings::IniFormat);
			QString strLastModuleDir = settings.value("history/strLastSaveDir","./").toString();
			QString dirstr = strLastModuleDir;
			QFileInfo infofm(dv->fullFileName());
			QString newfm;
			if (infofm.exists())
			{
				dirstr = infofm.absoluteFilePath();
				newfm = infofm.absoluteFilePath();
			}
			else
			{
				newfm = QFileDialog::getSaveFileName(this,
													 tr("Save project"),
													 dirstr,
													 "tbj files (*.tbj);;All files(*.*)"
													 );
			}

			if (newfm.size()>2)
			{
				QFileInfo info(newfm);
				settings.setValue("history/strLastSaveDir",info.absolutePath());
				QByteArray json = dv->project()->toJson();
				QFile fo(newfm);
				if (fo.open(QIODevice::WriteOnly))
				{
					QFileInfo info(newfm);
					dv->setWindowTitle(info.completeBaseName());
					fo.write(json);
					fo.close();
					dv->setFullFileName(newfm);
					dv->set_modified(false);
					m_activePagesFileName.remove(oldfm);
					m_activePagesFileName[newfm] = sub;
				}
				else
					QMessageBox::warning(this,tr("Save Failed"),fo.errorString());
			}
		}
	}

}

void taskBusPlatformFrm::on_action_Save_Project_As_triggered()
{
	QMdiSubWindow * sub = ui->mdiArea->activeSubWindow();
	if (sub)
	{
		PDesignerView * dv = qobject_cast<PDesignerView *>(sub->widget());
		QString oldfm = dv->fullFileName();
		if (dv)
		{
			QSettings settings(inifile(),QSettings::IniFormat);
			QString strLastModuleDir = settings.value("history/strLastSaveDir","./").toString();
			QString dirstr = strLastModuleDir;
			QFileInfo infofm(dv->fullFileName());
			if (infofm.exists())
				dirstr = infofm.absoluteFilePath();
			QString newfm = QFileDialog::getSaveFileName(this,
														 tr("Save project"),
														 dirstr,
														 "tbj files (*.tbj);;All files(*.*)"
														 );

			if (newfm.size()>2)
			{
				QFileInfo info(newfm);
				settings.setValue("history/strLastSaveDir",info.absolutePath());
				QByteArray json = dv->project()->toJson();
				QFile fo(newfm);
				if (fo.open(QIODevice::WriteOnly))
				{
					QFileInfo info(newfm);
					dv->setWindowTitle(info.completeBaseName());
					fo.write(json);
					fo.close();
					dv->setFullFileName(newfm);
					dv->set_modified(false);
					m_activePagesFileName.remove(oldfm);
					m_activePagesFileName[newfm] = sub;
				}
				else
					QMessageBox::warning(this,tr("Save Failed"),fo.errorString());
			}
		}
	}
}


void taskBusPlatformFrm::slot_openprj(QString newfm)
{
	QFile fo(newfm);
	if (m_activePagesFileName.contains(newfm)==true)
		ui->mdiArea->setActiveSubWindow(m_activePagesFileName[newfm]);
	else if (fo.open(QIODevice::ReadOnly))
	{
		QByteArray ar = fo.readAll();
		QFileInfo info(newfm);
		PDesignerView * dv = new PDesignerView();
		if (dv)
		{
			QMdiSubWindow * wnd =  ui->mdiArea->addSubWindow(dv);
			dv->project()->fromJson(ar,this->m_toolModules[tr("All")]);
			dv->setWindowTitle(info.completeBaseName());
			dv->show();
			connect (dv,&PDesignerView::sig_showProp,this,&taskBusPlatformFrm::slot_showPropModel,Qt::QueuedConnection);
			connect (dv,&PDesignerView::sig_message,this,&taskBusPlatformFrm::slot_showMsg,Qt::QueuedConnection);
			connect (dv,&PDesignerView::sig_openprj,this,&taskBusPlatformFrm::slot_openprj,Qt::QueuedConnection);
			connect (dv,&PDesignerView::sig_projstarted,this,&taskBusPlatformFrm::slot_projstarted,Qt::QueuedConnection);
			connect (dv,&PDesignerView::sig_projstopped,this,&taskBusPlatformFrm::slot_projstopped,Qt::QueuedConnection);
			connect (dv,&PDesignerView::sig_closed,this,&taskBusPlatformFrm::slot_projclosed,Qt::QueuedConnection);
			QCoreApplication::processEvents();
			dv->project()->refresh_idxes();
			QCoreApplication::processEvents();
			dv->drawAll();
			m_activePagesFileName[newfm] = wnd;
			dv->setFullFileName(newfm);
		}

		fo.close();
	}
}
void taskBusPlatformFrm::taskBusPlatformFrm::on_action_Open_Project_triggered()
{
	QSettings settings(inifile(),QSettings::IniFormat);
	QString strLastModuleDir = settings.value("history/strLastOpenDir","./").toString();
	QString newfm = QFileDialog::getOpenFileName(this,tr("Open project"),strLastModuleDir,
												 "tbj files (*.tbj);;All files(*.*)"
												 );
	if (newfm.size()>2)
	{
		QFileInfo info(newfm);
		settings.setValue("history/strLastOpenDir",info.absolutePath());

		slot_openprj(newfm);
	}

}
/*!
 * \brief taskBusPlatformFrm::slot_projstarted
 * When project started, we should disable any UI events.
 */
void taskBusPlatformFrm::slot_projstarted()
{
	PDesignerView * dv = qobject_cast<PDesignerView *>(sender());
	if (dv)
	{
		QMdiSubWindow * sub = ui->mdiArea->activeSubWindow();
		bool needUp = false;
		if (!sub)
			needUp = true;
		else
		{
			PDesignerView * dvc = qobject_cast<PDesignerView *>(sub->widget());
			if (dvc==dv)
				needUp = true;
		}
		if (needUp)
		{
			ui->action_Start_project->setEnabled(!dv->is_running());
			ui->action_stop_project->setEnabled(dv->is_running());
			ui->dockWidget_props->setEnabled(!dv->is_running());
			dv->setEnabled(!dv->is_running());
		}
	}
}

/*!
 * \brief taskBusPlatformFrm::slot_projstopped
 * When project stopped, we shall  enable UI
 */
void taskBusPlatformFrm::slot_projstopped()
{
	PDesignerView * dv = qobject_cast<PDesignerView *>(sender());
	if (dv)
	{
		QMdiSubWindow * sub = ui->mdiArea->activeSubWindow();
		bool needUp = false;
		if (!sub)
			needUp = true;
		else
		{
			PDesignerView * dvc = qobject_cast<PDesignerView *>(sub->widget());
			if (dvc==dv)
				needUp = true;
		}
		if (needUp)
		{
			ui->action_Start_project->setEnabled(!dv->is_running());
			ui->action_stop_project->setEnabled(dv->is_running());
			ui->dockWidget_props->setEnabled(!dv->is_running());
			dv->setEnabled(!dv->is_running());
		}
	}
}

void taskBusPlatformFrm::slot_projclosed(QString fm)
{
	qDebug()<<fm<<" closed.";
	m_activePagesFileName.remove(fm);
}
