#ifndef TGRAPHICSTASKITEM_H
#define TGRAPHICSTASKITEM_H
#include <QGraphicsItem>
#include <QVector>
#include <QPointF>
#include <QMap>
class PDesignerView;
class taskModule;
/*!
 * \brief The TGraphicsTaskItem class
 * TGraphicsTaskItem 绘制类似集成电路式样的单元图标
 * Draw a unit icon similar to the design of an integrated circuit
 */
class TGraphicsTaskItem : public QGraphicsItem
{
public:
	/*!
	 * \brief The pin_info struct
	 * 内部结构体，用于描述管脚的顺序、名称。
	 * An internal structure used to describe the order and name of the pins.
	 */
	struct pin_info{
		taskModule * pModule;
		bool bInPin;
		int  nOrder;
		QString sName;
		bool operator == (const pin_info & T) const
		{
			return T.pModule==pModule && T.bInPin == bInPin &&
					T.nOrder==nOrder && T.sName==sName;
		}
	};
public:
	TGraphicsTaskItem(PDesignerView * pv,taskModule * pm,QGraphicsItem * para = nullptr);
public:
	QRectF boundingRect() const override;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR) override;
public:
	//返回各个接口的位置 return position for the cell
	QPointF in_subject_pos(QString name);
	QPointF out_subject_pos(QString name);
	QColor in_subject_color(QString name);
protected:
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
	void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
	void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
protected:
	PDesignerView * m_pPrjView = nullptr;
	taskModule * m_pModel = nullptr;
	QMap<QString,QPointF> m_insbpos;
	QMap<QString,QPointF> m_outsbpos;
	QMap<QString,QColor> m_icol;
	int cellSize(QMap<QString,int> *mp_SrcPos, QMap<QString,int> *mp_DstPos) const;
public:
	static QSet<pin_info> m_pinList;
};

uint qHash(const TGraphicsTaskItem::pin_info & key, uint seed = 0);

#endif // TGRAPHICSTASKITEM_H

