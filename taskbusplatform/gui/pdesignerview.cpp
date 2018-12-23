#include "pdesignerview.h"
#include "ui_pdesignerview.h"
#include <QDragEnterEvent>
#include <QMimeData>
#include <QDropEvent>
#include <QDebug>
#include <QFileInfo>
#include <QMdiSubWindow>
#include <QDataStream>
#include <QClipboard>
#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QMessageBox>
#include <QJsonObject>
#include <QEasingCurve>
#include <QToolBar>
#include "core/tasknode.h"
#include "taskmodule.h"
#include "tgraphicstaskitem.h"
#include "tb_interface.h"
#include "core/process_prctl.h"


int PDesignerView::m_nextCV = 1;

PDesignerView::PDesignerView(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::PDesignerView),
	m_scene(new QGraphicsScene(0,0,4096,3072,this)),
	m_pRunThread(new QThread(this)),
	m_project(new taskProject(0))
{
	ui->setupUi(this);
	setAcceptDrops(true);

	connect(this,&PDesignerView::sig_updatePaths,this,&PDesignerView::update_paths,Qt::QueuedConnection);

	ui->graphicsView_main->setScene(m_scene);

	//工程管理，用来控制显示
	//project's call back funtions to control with display.
	m_project->setCallback_InsAppended(std::bind(&PDesignerView::callbk_instanceAppended,
												 this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
	m_project->setCallback_NewCell(std::bind(&PDesignerView::callbk_newcell,this));
	m_project->setCallback_IndexRefreshed(std::bind(&PDesignerView::callbk_refreshIdx,this));
	m_project->setCallback_GetCellPos(std::bind(&PDesignerView::callbk_GetCellPos,this,std::placeholders::_1));
	//工程管理，用来控制多线程
	//Project management .
	connect (m_project, &taskProject::sig_message, this, &PDesignerView::sig_message,Qt::QueuedConnection );
	connect (m_project, &taskProject::sig_stopped, this, &PDesignerView::slot_project_stopped,Qt::QueuedConnection );
	connect (m_project, &taskProject::sig_started, this, &PDesignerView::slot_project_started,Qt::QueuedConnection );
	connect (this, &PDesignerView::cmd_start_project,m_project,&taskProject::start_project,Qt::QueuedConnection);
	connect (this, &PDesignerView::cmd_stop_project, m_project,
			 static_cast<void (taskProject::*)(QThread *)>(&taskProject::stop_project),Qt::QueuedConnection);

	//创建工具栏 toolbar
	QToolBar * bar = new QToolBar(tr("zoom"),this) ;
	bar->addAction(ui->actionzoom_In);
	bar->addAction(ui->actionzoom_Out);
	bar->addAction(ui->actionzoom_orgin);
	bar->addSeparator();
	bar->addAction(ui->actionCopy);
	bar->addAction(ui->actionCut);
	bar->addAction(ui->actionPaste);
	bar->addSeparator();
	bar->addAction(ui->actionConnectLine);
	bar->addAction(ui->actionDeleteLine);
	bar->addAction(ui->actionPinUp);
	bar->addAction(ui->actionPinDown);
	bar->addAction(ui->actionPinSide);
	bar->addSeparator();
	bar->addAction(ui->actionDelete_selected_node);
	bar->addAction(ui->actiondebug_on);
	bar->addAction(ui->actiondebug_off);
	bar->addAction(ui->actionNiceUp);
	bar->addAction(ui->actionNiceDown);
	bar->adjustSize();
	bar->show();

	bar->setIconSize(QSize(24,24));

	m_pRunThread->start();
}

PDesignerView::~PDesignerView()
{
	m_project->stop_project();
	foreach (QGraphicsItem * it, m_vec_gitems)
		delete it;
	m_pRunThread->quit();
	m_pRunThread->wait();
	m_project->deleteLater();
	delete ui;
}

void PDesignerView::callbk_refreshIdx()
{
	emit sig_updatePaths();
	set_modified();
}

void PDesignerView::zoomIn()
{
	ui->graphicsView_main->scale(1.2,1.2);
}
void PDesignerView::zoomOut()
{
	ui->graphicsView_main->scale(0.8,0.8);
}

void PDesignerView::callbk_instanceAppended(taskCell * pmod, taskNode * node,QPointF pt)
{
	taskModule * mod = dynamic_cast<taskModule *>(pmod);
	if (mod)
	{
		//QString vname = mod->function_names().first();
		//Add graphics item
		TGraphicsTaskItem * git = new TGraphicsTaskItem(this,mod);
		git->moveBy(pt.x(),pt.y());
		m_scene->addItem(git);
		m_vec_gitems.push_back(git);
		connect (mod, &QAbstractItemModel::dataChanged, this->m_project, &taskProject::refresh_idxes,Qt::QueuedConnection);
		set_modified();
	}
}

taskCell * PDesignerView::callbk_newcell()
{
	return new taskModule(m_project);
}
/*!
 * \brief PDesignerView::run
 * 为了保证密集的IO不会阻塞界面，每个工程的IO吞吐是在独立的线程运行的。由于工程本身又
 * 和GUI相关，因此在工程启动时，事件会被“转移（moveToThread）”到独立线程；工程结束运行，
 * 则重新回到GUI线程。
 *
 * To ensure that dense IO does not block the interface, IO throughput for
 * each project runs on a separate thread. Because the project itself is related
 * to the GUI, when the project starts, the event is
 * "transferred (Movetothread)" to the stand-alone thread, and when the project
 * is finished running, it returns to the GUI thread.
 */
void PDesignerView::run()
{
	if (modified())
	{
		if (QMessageBox::information(this,tr("Save?"),tr("Project has been modified, continue without saving ?"),
									 QMessageBox::Ok,
									 QMessageBox::Cancel)!=QMessageBox::Ok)
		{
			return;
		}
	}

	m_project->moveToThread(m_pRunThread);
	emit cmd_start_project();
}

bool PDesignerView::is_running()
{
	return m_project->isRunning();
}

void PDesignerView::stop()
{
	emit cmd_stop_project(this->thread());
}
void PDesignerView::slot_project_stopped()
{
	setEnabled(true);
	emit sig_projstopped();
}
void PDesignerView::slot_project_started()
{
	setEnabled(false);
	emit sig_projstarted();
}
/*!
 * \brief PDesignerView::dragEnterEvent 用于测试是否有合法的构件被拖入
 * test whether there is a valid module deing dragging into this scene.
 * \param event 构建的事件。 Drag event
 * \ref taskModule::mimeData
 */
void PDesignerView::dragEnterEvent(QDragEnterEvent *event)
{
	QByteArray encodedData = event->mimeData()->data("application/vnd.text.list");
	QDataStream stream(&encodedData, QIODevice::ReadOnly);
	QStringList newItems;
	int rows = 0;

	while (!stream.atEnd()) {
		QString text;
		stream >> text;
		newItems << text;
		++rows;
	}
	if (newItems.size())
		event->acceptProposedAction();

}
/*!
 * \brief PDesignerView::dropEvent 添加一个构件到当前场景。Drop a module into scene
 * \param event
 * \ref	taskModule::mimeData
 */
void PDesignerView::dropEvent(QDropEvent * event)
{
	QByteArray encodedData = event->mimeData()->data("application/vnd.text.list");
	QDataStream stream(&encodedData, QIODevice::ReadOnly);
	QStringList newItems;
	int rows = 0;

	while (!stream.atEnd()) {
		QString text;
		stream >> text;
		newItems << text;
		++rows;
	}
	if (newItems.size())
	{
		event->accept();
		foreach (QString md, newItems)
		{
			QPoint pt = event->pos();
			QPoint pt2 = ui->graphicsView_main->mapFromParent(pt);
			QPointF pt3 = ui->graphicsView_main->mapToScene(pt2);
			m_project->add_node(md,pt3);
		}
		set_modified();
	}
}

void  PDesignerView::addCell(QMimeData * data)
{
	QByteArray encodedData = data->data("application/vnd.text.list");
	QDataStream stream(&encodedData, QIODevice::ReadOnly);
	QStringList newItems;
	int rows = 0;

	while (!stream.atEnd()) {
		QString text;
		stream >> text;
		newItems << text;
		++rows;
	}
	if (newItems.size())
	{
		static int offset = 0;
		foreach (QString md, newItems)
		{
			QPointF pt3 = ui->graphicsView_main->sceneRect().center();
			pt3.setX(pt3.x()-200+((offset%25)/5)*100);
			pt3.setY(pt3.y()-200+((offset%25)%5)*100);
			if (pt3.x()<0)	pt3.setX(300);
			if (pt3.y()<0)	pt3.setY(300);
			if (pt3.x()>m_scene->width())	pt3.setX(m_scene->width()-300);
			if (pt3.y()>m_scene->height())	pt3.setY(m_scene->height()-300);
			m_project->add_node(md,pt3);
			++offset;
		}
		set_modified();
	}

}

void PDesignerView::closeEvent(QCloseEvent * e)
{
	if (modified())
	{
		if (QMessageBox::information(this,tr("Close without Saving?"),tr("Project has been modified, Close it anyway?"),
									 QMessageBox::Ok,
									 QMessageBox::Cancel)!=QMessageBox::Ok)
		{
			e->ignore();
			return;
		}
	}
	emit sig_closed(fullFileName());
	this->stop();
	e->accept();
}

void PDesignerView::show_prop_page(QObject * model)
{
	emit sig_showProp(model);
}

/*!
 * \brief PDesignerView::update_paths 根据出入专题的号码，绘制连接线
 * draw lines between pins(subjects)
 */
void PDesignerView::update_paths()
{
	m_idx_in2pos.clear();
	m_idx_out2pos.clear();
	m_idx_in2color.clear();

	const int sizeV = m_project->vec_nodes().size();
	//刷新各个接口的图形位置
	//Update each pins' pos
	for (int i=0;i<sizeV;++i)
	{
		const taskCell * model = m_project->vec_cells()[i];
		m_vec_gitems[i]->update();
		const QStringList lstfuns = model->function_names();
		if (lstfuns.size()!=1)
			continue;
		const QString func = lstfuns.first();
		//in
		const QStringList lstSrcSubs = model->in_subjects(func);
		foreach (QString bname,lstSrcSubs )
		{
			const unsigned int subid = model->in_subject_instance(func,bname);
			if (subid)
			{
				m_idx_in2pos[subid].push_back(m_vec_gitems[i]->in_subject_pos(bname));
				m_idx_in2color[subid].push_back(m_vec_gitems[i]->in_subject_color(bname));
			}
		}
		//out
		const QStringList lstOutSubs = model->out_subjects(func);
		foreach (QString bname,lstOutSubs )
		{
			const unsigned int subid = model->out_subject_instance(func,bname);
			if (subid)
				m_idx_out2pos[subid].push_back(m_vec_gitems[i]->out_subject_pos(bname));
		}
	}

	foreach (QGraphicsLineItem * it, m_lines)
	{
		m_scene->removeItem(it);
		delete it;
	}
	m_lines.clear();

	const QList<unsigned int> srcs = m_project->idx_in2instances().keys();
	const QList<unsigned int> dsts =  m_project->idx_out2instances().keys();
	const QSet<unsigned int> setin =  QSet<unsigned int>::fromList(srcs);
	const QSet<unsigned int> setout =  QSet<unsigned int>::fromList(dsts);

	QEasingCurve cuve(QEasingCurve::InOutCubic);

	foreach (unsigned int pins, setin)
	{
		if (setout.contains(pins))
		{
			const int szSrc = m_idx_in2pos[pins].size();
			const int szDst = m_idx_out2pos[pins].size();
			for (int snc = 0;snc < szSrc;++snc)
			{
				const QPointF srcp = m_idx_in2pos[pins][snc];
				const QColor scolor = m_idx_in2color[pins][snc];
				for (int dnc = 0;dnc < szDst;++dnc)
				{
					const QPointF drcp = m_idx_out2pos[pins][dnc];
					if (srcp.x()<=10||srcp.y()<=10||drcp.x()<=10||drcp.y()<=10)
						continue;

					int ccr = scolor.red(), ccg = scolor.green(), ccb = scolor.blue();
					using std::max;
					int maxc = max(max(ccr,ccg),ccb);
					if (ccr<maxc)  ccr /=3 ;else ccr /=1.3;
					if (ccg<maxc)  ccg /=3 ;else ccg /=1.3;
					if (ccb<maxc)  ccb /=3 ;else ccb /=1.3;
					const QColor c(ccr,ccg,ccb,128);
					const int linesp = 16;
					for (int bb = 0; bb<16;++bb)
					{
						const double xp1 = bb*1.0/linesp, xp2 = (bb+1)*1.0/linesp;
						const double vp1 = cuve.valueForProgress(xp1);
						const double vp2 = cuve.valueForProgress(xp2);
						QGraphicsLineItem * item = new QGraphicsLineItem();
						item->setLine(srcp.x() + xp1 * (drcp.x()-srcp.x())+.5,
									  srcp.y() + vp1 * (drcp.y()-srcp.y())+.5,
									  srcp.x() + xp2 * (drcp.x()-srcp.x())+.5,
									  srcp.y() + vp2 * (drcp.y()-srcp.y())+.5
									  );
						item->setPen(QPen(QBrush(c),2,Qt::DashLine));
						m_lines<<item;
						m_scene->addItem(item);
					}

				}
			}
		}
	}
}
int PDesignerView::selectedNode()
{
	int sz = m_vec_gitems.size();
	for (int i=0;i<sz;++i)
	{
		if (m_vec_gitems[i]->isSelected())
			return i;
	}
	return -1;
}
void PDesignerView::deleteNode(int node)
{
	int sz = m_vec_gitems.size();
	if (node>=0 && node < sz)
	{
		//清理图元 delete a graphices item
		m_scene->removeItem(m_vec_gitems[node]);
		delete m_vec_gitems[node];
		m_vec_gitems.remove(node);
		//删除运行时 delete the runtime cell and node
		m_project->del_node(node);
		set_modified();
	}
}

void PDesignerView::debug_node(int node, bool on)
{
	int sz = m_vec_gitems.size();
	if (node>=0 && node < sz)
	{
		if (on)
			m_project->turnOnDebug(node);
		else
			m_project->turnOffDebug(node);
	}
	ui->graphicsView_main->scale(0.5,1);
	ui->graphicsView_main->scale(2,1);


}



bool PDesignerView::is_debug_node(int node)
{
	int sz = m_vec_gitems.size();
	if (node>=0 && node < sz)
	{
		return m_project->vec_nodes()[node]->isDebug();
	}
	return false;

}


QPointF PDesignerView::callbk_GetCellPos(int ncell)
{
	//位置
	QPointF pf = m_vec_gitems[ncell]->pos();
	return pf;
}
void PDesignerView::open_project(QString fm)
{
	emit sig_openprj(fm);
}
void PDesignerView::drawAll()
{
	ui->graphicsView_main->scale(0.1,0.1);
	QCoreApplication::processEvents();
	update_paths();
	QCoreApplication::processEvents();
	ui->graphicsView_main->resetTransform();
}

void PDesignerView::on_actionzoom_In_triggered()
{
	zoomIn();
}

void PDesignerView::on_actionzoom_Out_triggered()
{
	zoomOut();
}

void PDesignerView::on_actionzoom_orgin_triggered()
{
	ui->graphicsView_main->resetTransform();
}

void PDesignerView::on_actionDelete_selected_node_triggered()
{
	deleteNode(selectedNode());
}

void PDesignerView::on_actiondebug_on_triggered()
{
	debug_node(selectedNode(),true);
}

void PDesignerView::on_actiondebug_off_triggered()
{
	debug_node(selectedNode(),false);
}

void PDesignerView::on_actionCopy_triggered()
{
	const int node = selectedNode();
	int sz = m_vec_gitems.size();
	if (node>=0 && node < sz)
	{
		taskCell * tmod = m_project->vec_cells()[node];

		QClipboard *clipboard = QGuiApplication::clipboard();
		QString newText = tmod->toJson("");
		clipboard->setText(newText);

	}
}

void PDesignerView::on_actionPaste_triggered()
{
	QClipboard *clipboard = QGuiApplication::clipboard();
	QString text = clipboard->text();

	QPointF pt = ui->graphicsView_main->sceneRect().center();
	m_project->add_node(text,pt);
	set_modified();
}

void PDesignerView::on_actionCut_triggered()
{
	const int node = selectedNode();
	int sz = m_vec_gitems.size();
	if (node>=0 && node < sz)
	{
		taskCell * tmod = m_project->vec_cells()[node];

		QClipboard *clipboard = QGuiApplication::clipboard();
		QString newText = tmod->toJson("");
		clipboard->setText(newText);
		deleteNode(node);
	}
}

void PDesignerView::on_actionConnectLine_triggered()
{
	int gIns = 0;
	foreach (TGraphicsTaskItem::pin_info p, TGraphicsTaskItem::m_pinList)
	{
		if (p.pModule)
		{
			int ins = 0;
			if (p.bInPin)
				ins = p.pModule->in_subject_instance(p.pModule->function_firstname(),p.sName);
			else
				ins = p.pModule->out_subject_instance(p.pModule->function_firstname(),p.sName);
			if (ins>0)
			{
				gIns = ins;
				break;
			}
		}
	}
	if (gIns==0)
	{
		while (taskModule::m_pinInsValues.contains(m_nextCV))
			++m_nextCV;
		gIns = m_nextCV++;
	}
	foreach (TGraphicsTaskItem::pin_info p, TGraphicsTaskItem::m_pinList)
	{
		if (p.pModule)
		{
			if (p.bInPin)
				p.pModule->set_in_subject_instance(p.pModule->function_firstname(),p.sName,gIns);
			else
				p.pModule->set_out_subject_instance(p.pModule->function_firstname(),p.sName,gIns);
			set_modified();
		}
	}

	TGraphicsTaskItem::m_pinList.clear();
	m_project->refresh_idxes();
	ui->graphicsView_main->scale(0.5,1);
	ui->graphicsView_main->scale(2,1);
}

void PDesignerView::on_actionDeleteLine_triggered()
{
	foreach (TGraphicsTaskItem::pin_info p, TGraphicsTaskItem::m_pinList)
	{
		if (p.pModule)
		{
			if (p.bInPin)
				p.pModule->set_in_subject_instance(p.pModule->function_firstname(),p.sName,0);
			else
				p.pModule->set_out_subject_instance(p.pModule->function_firstname(),p.sName,0);
		}
		set_modified();
	}
	TGraphicsTaskItem::m_pinList.clear();
	m_project->refresh_idxes();
	ui->graphicsView_main->scale(0.5,1);
	ui->graphicsView_main->scale(2,1);
}

void PDesignerView::on_actionPinUp_triggered()
{
	foreach (TGraphicsTaskItem::pin_info p, TGraphicsTaskItem::m_pinList)
	{
		if (p.pModule)
		{
			QString func = p.pModule->function_firstname();
			if (p.bInPin)
			{
				int dire = p.pModule->draw_direction(func,true,p.nOrder);
				if (dire <=0)
					dire--;
				else
					dire++;
				p.pModule->set_draw_direction(func,true,p.nOrder,dire);
			}
			else
			{
				int dire = p.pModule->draw_direction(func,false,p.nOrder);
				if (dire <=0)
					dire--;
				else
					dire++;
				p.pModule->set_draw_direction(func,false,p.nOrder,dire);

			}
		}
	}
	m_project->refresh_idxes();
	ui->graphicsView_main->scale(0.5,1);
	ui->graphicsView_main->scale(2,1);
	set_modified();

}

void PDesignerView::on_actionPinDown_triggered()
{
	foreach (TGraphicsTaskItem::pin_info p, TGraphicsTaskItem::m_pinList)
	{
		if (p.pModule)
		{
			QString func = p.pModule->function_firstname();
			if (p.bInPin)
			{
				int dire = p.pModule->draw_direction(func,true,p.nOrder);
				if (dire <=0)
				{
					dire++;
					if (dire>0)
					{
						dire = 0;
						QMessageBox::information(this,tr("Already at min"),tr("Pin order value already at min, you can move other pins UP"),
												 QMessageBox::Ok);
					}
				}
				else
				{
					dire--;
					if (dire<=0)
					{
						dire = 1;
						QMessageBox::information(this,tr("Already at min"),tr("Pin order value already at min, you can move other pins UP"),
												 QMessageBox::Ok);
					}
				}
				p.pModule->set_draw_direction(func,true,p.nOrder,dire);
			}
			else
			{
				int dire = p.pModule->draw_direction(func,false,p.nOrder);
				if (dire <=0)
				{
					dire++;
					if (dire>0)
					{
						dire = 0;
						QMessageBox::information(this,tr("Already at min"),tr("Pin order value already at min, you can move other pins UP"),
												 QMessageBox::Ok);
					}
				}
				else
				{
					dire--;
					if (dire<=0)
					{
						dire = 1;
						QMessageBox::information(this,tr("Already at min"),tr("Pin order value already at min, you can move other pins UP"),
												 QMessageBox::Ok);
					}

				}
				p.pModule->set_draw_direction(func,false,p.nOrder,dire);

			}
		}
	}
	m_project->refresh_idxes();
	ui->graphicsView_main->scale(0.5,1);
	ui->graphicsView_main->scale(2,1);
	set_modified();
}

void PDesignerView::on_actionPinSide_triggered()
{
	foreach (TGraphicsTaskItem::pin_info p, TGraphicsTaskItem::m_pinList)
	{
		if (p.pModule)
		{
			QString func = p.pModule->function_firstname();
			if (p.bInPin)
			{
				int dire = -p.pModule->draw_direction(func,true,p.nOrder);
				if (dire==0)
					dire = 1;
				p.pModule->set_draw_direction(func,true,p.nOrder,dire);
			}
			else
			{
				int dire = -p.pModule->draw_direction(func,false,p.nOrder);
				if (dire==0)
					dire = 1;
				p.pModule->set_draw_direction(func,false,p.nOrder,dire);

			}
		}
	}
	m_project->refresh_idxes();
	ui->graphicsView_main->scale(0.5,1);
	ui->graphicsView_main->scale(2,1);
	set_modified();
}

void PDesignerView::on_actionNiceUp_triggered()
{
	int sz = m_vec_gitems.size();
	const int node = selectedNode();
	if (node>=0 && node < sz)
	{
		int nic = project()->get_nice(node);
		++nic;
		if (nic>TASKBUS::pnice_max)
			nic = TASKBUS::pnice_max;
		project()->set_nice(node,nic);
	}
	ui->graphicsView_main->scale(0.5,1);
	ui->graphicsView_main->scale(2,1);
	set_modified();
}

void PDesignerView::on_actionNiceDown_triggered()
{
	int sz = m_vec_gitems.size();
	const int node = selectedNode();
	if (node>=0 && node < sz)
	{
		int nic = project()->get_nice(node);
		--nic;
		if (nic<TASKBUS::pnice_min)
			nic = TASKBUS::pnice_min;
		project()->set_nice(node,nic);
	}
	ui->graphicsView_main->scale(0.5,1);
	ui->graphicsView_main->scale(2,1);
	set_modified();
}

void PDesignerView::set_modified(bool bmod /*= true*/)
{
	if (m_bModified!=bmod && m_strFullFilename.length())
	{
		QObject * wig = parent();
		QFileInfo info(m_strFullFilename);
		QString baseName = info.completeBaseName();
		if (wig)
		{
			QMdiSubWindow * wnd =  qobject_cast<QMdiSubWindow*>(wig);
			if (wnd)
				wnd->setWindowTitle(baseName + (bmod?"*":""));
		}
		m_bModified = bmod;
	}

}
