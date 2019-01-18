#include "tgraphicstaskitem.h"
#include <QPainter>
#include <QDebug>
#include <QFileInfo>
#include <QList>
#include <algorithm>
#include <QGraphicsSceneMouseEvent>
#include "pdesignerview.h"
#include "taskmodule.h"

QSet<TGraphicsTaskItem::pin_info> TGraphicsTaskItem::m_pinList;
TGraphicsTaskItem::TGraphicsTaskItem(PDesignerView * pv,taskModule * pm,QGraphicsItem * para )
	:QGraphicsItem(para),
	 m_pPrjView(pv),
	 m_pModel(pm)
{
	setFlags(QGraphicsItem::ItemIsMovable|QGraphicsItem::ItemIsSelectable );
	setCursor(QCursor(Qt::CrossCursor));
}

QRectF TGraphicsTaskItem::boundingRect() const
{
	const QString func = m_pModel->function_names().first();
	const QStringList lstSrcPs = m_pModel->in_subjects(func);
	const QStringList lstDstPs = m_pModel->out_subjects(func);
	const int szz = cellSize(0,0)+1;
	const int width_cell = 300;
	const int height_cell = 32 * szz;
	return QRectF(-width_cell/2,-height_cell/2,width_cell,height_cell);
}

void TGraphicsTaskItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	const  QString func = m_pModel->function_names().first();
	const  QStringList lstSrcPs = m_pModel->in_subjects(func);
	const  QStringList lstDstPs = m_pModel->out_subjects(func);
	const  QString tooltip = m_pModel->function_tooltip(func);
	const  QString className = taskCell::className(func);
	const  unsigned int instance_id = m_pModel->function_instance(func);

	QMap<QString,int> mp_Src,mp_Dst;

	const int cell_size_raw = cellSize(&mp_Src,&mp_Dst)+1;
	const int cell_size = cell_size_raw<2?2:cell_size_raw;
	const int cell_width = 300;
	const int cell_height = 32 * cell_size;
	QPen pen = painter->pen();
	QPen newpen (QColor(255,0,0));
	QBrush bkbr(QColor(instance_id * 61%127+128 ,
									 instance_id * 121%127+128,
									 instance_id * 37%127+128,200));
	QBrush br_raw = painter->brush();
	painter->setBrush(bkbr);
	//选中 Selection
	if(isSelected())
		painter->setPen(newpen);

	painter->drawRect(QRectF(-50,-cell_height/2,100,cell_height));
	//文字 Names
	painter->drawText(-50+4,-cell_height/2 + 16 -4,tooltip);
	painter->drawText(-50+4,-cell_height/2 + 32 -4,QString("ID:%1").arg(instance_id));
	painter->drawText(-50+4,-cell_height/2 + 48 -4,className.size()?className:func);
	QMap<QString,QVariant> vm
			= m_pModel->additional_paras(func);
	int nic = 2;
	if (vm.contains("nice"))
		nic = vm["nice"].toInt();
	painter->drawText(-50+4,-cell_height/2 + 64 -4,QString("nice=%1").arg(nic));


	//谁的绝对值小，就在底下
	//Those pins with small absolute value is drawing downside.
	int ctInput = 0;
	foreach (QString src, lstSrcPs)
	{
		int direct = mp_Src[src];
		int szz = direct<0?-direct:direct;

		QString pname = m_pModel->in_subject_tooltip(func,src);
		unsigned int pins = m_pModel->in_subject_instance(func,src);
		QColor linec = QColor(pins * 61%127+128 ,
							  pins * 121%127+128,
							  pins * 37%127+128);

		QBrush newbr(linec);
		QBrush brold = painter->brush();
		painter->setBrush(newbr);


		pin_info info;
		info.bInPin = true;
		info.nOrder = ctInput;
		info.pModule = this->m_pModel;
		info.sName = src;
		if (m_pinList.contains(info))
			painter->setPen(QPen(QBrush(QColor(128,0,0)),4));


		if (direct<=0)
		{

			painter->drawText(-150,cell_height/2  - szz * 32+8,pname+QString("(%1)").arg(pins));
			painter->drawRect(-100,cell_height/2  - szz * 32+16,
							  50,8);
			painter->setBrush(brold);
			m_insbpos[src] = this->mapToScene(QPointF(-100,cell_height/2  - szz * 32+16+4));
			m_icol[src] = linec;
			QColor ash = QColor(0 ,	  0,	  0);
			painter->setPen(ash);
			painter->drawLine(-100+8,cell_height/2  - szz * 32+16+4
							  ,-100,cell_height/2  - szz * 32+16);
			painter->drawLine(-100+8,cell_height/2  - szz * 32+16+4
							  ,-100,cell_height/2  - szz * 32+16+8);
		}
		else
		{
			painter->drawText(+55,cell_height/2  - szz * 32+8,pname+QString("(%1)").arg(pins));
			painter->drawRect(+50,cell_height/2  - szz * 32+16,
							  50,8);
			painter->setBrush(brold);
			m_insbpos[src] = this->mapToScene(QPointF(100,cell_height/2  - szz * 32+16+4));
			m_icol[src] = linec;
			QColor ash = QColor(0 ,	  0,	  0);
			painter->setPen(ash);
			painter->drawLine(100-8,cell_height/2  - szz * 32+16+4
							  ,100,cell_height/2  - szz * 32+16);
			painter->drawLine(100-8,cell_height/2  - szz * 32+16+4
							  ,100,cell_height/2  - szz * 32+16+8);
		}
		++ctInput;
	}

	int ctOutput = 0;
	foreach (QString dst, lstDstPs)
	{
		int direct = mp_Dst[dst];
		int szz = direct<0?-direct:direct;

		QString pname = m_pModel->out_subject_tooltip(func,dst);
		int pouts = m_pModel->out_subject_instance(func,dst);
		QBrush newbr(QColor(pouts * 61%127+128 ,
										 pouts * 121%127+128,
										 pouts * 37%127+128));
		painter->setBrush(newbr);
		QBrush brold = painter->brush();

		pin_info info;
		info.bInPin = false;
		info.nOrder = ctOutput;
		info.pModule = this->m_pModel;
		info.sName = dst;
		if (m_pinList.contains(info))
			painter->setPen(QPen(QBrush(QColor(0,0,128)),4));


		if (direct>0)
		{
			painter->drawText(+55,cell_height/2  - szz * 32+8,pname+QString("\n(%1)").arg(pouts));
			painter->drawRect(+50,cell_height/2  - szz * 32+16,
							  50,8);
			painter->setBrush(brold);
			m_outsbpos[dst] = this->mapToScene(QPointF(100,cell_height/2  - szz * 32+16+4));
			QColor ash = QColor(0 ,	  0,	  0);
			painter->setPen(ash);
			painter->drawLine(100-8,cell_height/2  - szz * 32+16
							  ,100,cell_height/2  - szz * 32+16+4);
			painter->drawLine(100-8,cell_height/2  - szz * 32+16+8
							  ,100,cell_height/2  - szz * 32+16+4);
		}
		else
		{
			painter->drawText(-150,cell_height/2  - szz * 32+8,pname+QString("\n(%1)").arg(pouts));
			painter->drawRect(-100,cell_height/2  - szz * 32+16,
							  50,8);
			painter->setBrush(brold);
			m_outsbpos[dst] = this->mapToScene(QPointF(-100,cell_height/2  - szz * 32+16+4));
			QColor ash = QColor(0 ,	  0,	  0);
			painter->setPen(ash);
			painter->drawLine(-100+8,cell_height/2  - szz * 32+16
							  ,-100,cell_height/2  - szz * 32+16+4);
			painter->drawLine(-100+8,cell_height/2  - szz * 32+16+8
							  ,-100,cell_height/2  - szz * 32+16+4);
		}

		++ctOutput;
	}
	painter->setPen(pen);
	painter->setBrush(br_raw);

	if (m_pPrjView->project()->idx_instance2vec().contains(instance_id))
	{
		int nodev = m_pPrjView->project()->idx_instance2vec()[instance_id];
		if (m_pPrjView->is_debug_node(nodev))
			painter->drawText(-50,cell_height/2-32,"  DEBUG");
	}

}
void TGraphicsTaskItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if (event->buttons()&Qt::LeftButton)
	{
		//判断增补选择 select or un-select pins
		const  QString func = m_pModel->function_names().first();
		const  QStringList lstSrcPs = m_pModel->in_subjects(func);
		const  QStringList lstDstPs = m_pModel->out_subjects(func);
		const  unsigned int instance_id = m_pModel->function_instance(func);

		QMap<QString,int> mp_Src,mp_Dst;

		const int cell_size = cellSize(&mp_Src,&mp_Dst)+1;

		const double x = event->pos().x();
		const double y = event->pos().y();
		bool hit = false;
		int ctInput = 0;
		foreach (QString src, lstSrcPs)
		{
			const QPointF pos = this->mapFromScene(m_insbpos[src]);
			if (pos.x()<0)
			{
				if (x>=-100 && x<=-50 && y>=pos.y()-20 && y<=pos.y()+4)
				{
					pin_info info;
					info.bInPin = true;
					info.nOrder = ctInput;
					info.pModule = this->m_pModel;
					info.sName = src;
					if (m_pinList.contains(info))
						m_pinList.remove(info);
					else
						m_pinList.insert(info);
					hit = true;
					break;
				}
			}
			else
			{
				if (x>=50 && x<=100 && y>=pos.y()-20 && y<=pos.y()+4)
				{
					pin_info info;
					info.bInPin = true;
					info.nOrder = ctInput;
					info.pModule = this->m_pModel;
					info.sName = src;
					if (m_pinList.contains(info))
						m_pinList.remove(info);
					else
						m_pinList.insert(info);
					hit = true;
					break;
				}

			}
			++ctInput;
		}
		int ctOutput = 0;
		foreach (QString dst, lstDstPs)
		{
			const QPointF pos = this->mapFromScene(m_outsbpos[dst]);
			if (pos.x()<0)
			{
				if (x>=-100 && x<=-50 && y>=pos.y()-20 && y<=pos.y()+4)
				{
					pin_info info;
					info.bInPin = false;
					info.nOrder = ctOutput;
					info.pModule = this->m_pModel;
					info.sName = dst;
					if (m_pinList.contains(info))
						m_pinList.remove(info);
					else
						m_pinList.insert(info);
					hit = true;
					break;
				}
			}
			else
			{
				if (x>=50 && x<=100 && y>=pos.y()-20 && y<=pos.y()+4)
				{
					pin_info info;
					info.bInPin = false;
					info.nOrder = ctOutput;
					info.pModule = this->m_pModel;
					info.sName = dst;
					if (m_pinList.contains(info))
						m_pinList.remove(info);
					else
						m_pinList.insert(info);
					hit = true;
					break;
				}
			}
			++ctOutput;
		}
		if (hit==false)
		{
			m_pinList.clear();
		}
	}

	QGraphicsItem::mousePressEvent(event);
}
void TGraphicsTaskItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	qDebug()<<event->buttons();
	m_pPrjView->update_paths();
	m_pPrjView->show_prop_page(m_pModel);
	m_pPrjView->appendUndoList();
	QGraphicsItem::mouseReleaseEvent(event);
}
QPointF TGraphicsTaskItem::in_subject_pos(QString name)
{
	if (m_insbpos.contains(name))
		return m_insbpos[name];
	return QPointF();
}

QColor TGraphicsTaskItem::in_subject_color(QString name)
{
	if (m_icol.contains(name))
		return m_icol[name];
	return QColor(0,0,0);
}
QPointF TGraphicsTaskItem::out_subject_pos(QString name)
{
	if (m_outsbpos.contains(name))
		return m_outsbpos[name];
	return QPointF();

}
void TGraphicsTaskItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
	QGraphicsItem::mouseDoubleClickEvent(event);
	QString func = m_pModel->function_names().first();
	QString exe = m_pModel->function_exec(func);
	QFileInfo info(exe);
	QString strModFileName = info.absolutePath()+"/"+ info.completeBaseName()+".tbj";
	//2.加载工程
	m_pPrjView->open_project(strModFileName);
}

int TGraphicsTaskItem::cellSize(QMap<QString,int> *mp_SrcPos, QMap<QString,int> *mp_DstPos) const
{
	const  QString func = m_pModel->function_names().first();
	const  QStringList lstSrcPs = m_pModel->in_subjects(func);
	const  QStringList lstDstPs = m_pModel->out_subjects(func);
	QSet<int> usedPins[2];

	struct tmp_od
	{
		QString name;
		bool src;
		int absv;
		int value;

	};

	QList<tmp_od> list_pin[2];

	int ctInput = 0;
	int szz1 = 1;
	foreach (QString src, lstSrcPs)
	{
		int direct = m_pModel->draw_direction(func,true,ctInput);
		while (usedPins[direct<=0?0:1].contains(szz1))
			++szz1;
		usedPins[direct<=0?0:1].insert(szz1);
		++ctInput;
		tmp_od od;
		od.name = src;
		od.src = true;
		od.absv = direct<0?-direct:direct;
		od.value = direct;
		list_pin[direct<=0?0:1].push_back(od);
	}
	int ctOutput = 0;
	int szz2 = 1;
	foreach (QString dst, lstDstPs)
	{
		int direct = m_pModel->draw_direction(func,false,ctOutput);
		while (usedPins[direct<=0?0:1].contains(szz2))
			++szz2;
		usedPins[direct<=0?0:1].insert(szz2);
		++ctOutput;
		tmp_od od;
		od.name = dst;
		od.src = false;
		od.absv = direct<0?-direct:direct;
		od.value = direct;
		list_pin[direct<=0?0:1].push_back(od);
	}

	//排序
	std::sort(list_pin[0].begin(),list_pin[0].end(),[](const tmp_od & t1, const tmp_od & t2)->int {

		return t1.absv<t2.absv;
	});
	std::sort(list_pin[1].begin(),list_pin[1].end(),[](const tmp_od & t1, const tmp_od & t2)->int {

		return t1.absv<t2.absv;
	});
	int order[2] = {1,1};
	foreach (const tmp_od &o, list_pin[0]) {
		if (o.src==true)
		{
			int v = order[0]++;
			if (mp_SrcPos)
				(*mp_SrcPos)[o.name] = -v;
		}
		else
		{
			int v = order[0]++;
			if (mp_DstPos)
				(*mp_DstPos)[o.name] = -v;
		}
	}
	foreach (const tmp_od &o, list_pin[1]) {
		if (o.src==true)
		{
			int v = order[1]++;
			if (mp_SrcPos)
				(*mp_SrcPos)[o.name] = v;
		}
		else
		{
			int v = order[1]++;
			if (mp_DstPos)
				(*mp_DstPos)[o.name] = v;
		}
	}

	return std::max(szz1,szz2);
}

uint qHash(const TGraphicsTaskItem::pin_info & key, uint seed )
{
	uint h = qHash(key.nOrder,seed);
	h ^= qHash((quint64)key.pModule,seed);
	h ^= qHash(key.sName,seed);
	if (key.bInPin)
		h = ~h;
	return h;
}
