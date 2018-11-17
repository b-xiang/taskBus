#ifndef TaskMODULE_H
#define TaskMODULE_H

#include <QAbstractItemModel>
#include <QSet>
#include "core/taskcell.h"
/*!
 * \brief The TaskModule class 这个类用于承载运行时的属性参数，同时用于模块列表
 * 和选项卡（属性编辑窗口）。
 * This class is used to host property parameters for the runtime,
 * as well as for module lists and tabs (property editing Windows).
 */
class taskModule : public QAbstractItemModel, public taskCell
{
	Q_OBJECT

public:
	explicit taskModule(QObject *parent = nullptr);
	explicit taskModule(bool mbt,QObject *parent = nullptr);
	// Header:返回列的名称 Return table-view header names
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	// Basic functionality:
	QModelIndex index(int row, int column,
					  const QModelIndex &parent = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex &index) const override;

	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;

	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

	// Editable:
	bool setData(const QModelIndex &index, const QVariant &value,
				 int role = Qt::EditRole) override;

	Qt::ItemFlags flags(const QModelIndex& index) const override;

	//Drag
	QMimeData *mimeData(const QModelIndexList &indexes) const override;
	QStringList mimeTypes() const override;

	QModelIndex paraIndex();
	/*! 下面这些重载为了弥合	QAbstractItemModel 的刷新与通告。除此以外，只是简单
	 * 调用了 taskCell 的方法。
	  * Inside these overridings, taskCell::[methods] is simply called.
	  * QAbstractItemModel will send notifies to views if taskCell has been
	  * modified.
	*/
public:
	bool initFromJson(const QByteArray json, const QString path = "") override;
	void clear() override;
	unsigned int set_function_instance(const QString func, unsigned  int instance) override;//设置模块实例标识
	unsigned int set_in_subject_instance(const QString func,const QString name, unsigned int instance) override;//设置输入接口实例标识
	unsigned int set_out_subject_instance(const QString func,const QString name, unsigned int instance) override;//设置输出接口实例标识
	unsigned int in_subject_instance(const QString func, const QString name) const override;
	unsigned int out_subject_instance(const QString func, const QString name) const override;
	QVariant set_parameters_instance(const QString func,const QString name, const QVariant instance) override;//设置参数实例标识	
	void set_additional_paras(const QString func,const QMap<QString,QVariant> & p) override;
	//绘制朝向设置
	virtual int draw_direction(const QString func, bool bInput, int n)  const ;
	virtual void set_draw_direction(const QString func, bool bInput, int n, int direction) ;
public:
	/*!
	 * \brief m_pinInsValues 用于记录使用过的局部连线整数
	 * This set holds pin instance values that have been already used.
	 */
	static QSet<int> m_pinInsValues;
	bool m_btoolMod = false;
};

#endif // TaskMODULE_H
