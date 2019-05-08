#include "taskmodule.h"
#include <QJsonObject>
#include <QDebug>
#include <QJsonParseError>
#include <QMimeData>
#include <QBrush>
#include <QDataStream>

QSet<int> taskModule::m_pinInsValues;

taskModule::taskModule(QObject *parent)
	: QAbstractItemModel(parent)
{
}
taskModule::taskModule(bool mbt,QObject *parent)
	: QAbstractItemModel(parent)
	,m_btoolMod(mbt)
{

}

QVariant taskModule::headerData(int section, Qt::Orientation orientation, int role) const
{
	QString strHeaderNames[7] = {tr("Module Name"),tr("Class"),tr("Key"),tr("InstanceValue"),tr("Type"),tr("DefaultValue"),tr("Range")};
	if (role==Qt::DisplayRole || role==Qt::EditRole)
	{
		if (orientation==Qt::Horizontal)
		{
			if (section>=0 && section<7 && m_btoolMod==false)
				return  strHeaderNames[section];
			if (section>=0 && section<2 && m_btoolMod==true)
				return  strHeaderNames[1-section];
		}
		return  QString("%1").arg(section);
	}
	return QAbstractItemModel::headerData(section,orientation,role);
}


QModelIndex taskModule::index(int row, int column, const QModelIndex &parent) const
{
	tag_titem * node = nullptr;
	if (m_idxs.size()==0)
		return QModelIndex();
	if (parent.isValid())
		node = (tag_titem *)parent.internalPointer();
	else
		node = m_idxs.first().get();
	if (row<0 || row >node->children.size())
		return QModelIndex();
	return createIndex(row,column,node->children[row]);
}

QModelIndex taskModule::parent(const QModelIndex &index) const
{
	tag_titem * node = nullptr;
	if (m_idxs.size()==0)
		return QModelIndex();
	if (index.isValid())
		node = (tag_titem *)index.internalPointer();

	if (node)
	{
		tag_titem * node_para = node->parent;
		if (node_para==nullptr)
			return QModelIndex();
		if (m_idxparas.contains(node_para))
			return createIndex(m_idxparas[node_para],0,node_para);
	}
	return QModelIndex();
}

int taskModule::rowCount(const QModelIndex &parent) const
{
	if (m_idxs.size()==0)
		return 0;
	if (!parent.isValid())
		return m_idxs[0]->children.size();

	tag_titem * node = (tag_titem *)parent.internalPointer();
	if (m_idxs.size()==0)
		return 0;

	if (node)
		return node->children.size();
	return 0;
}

int taskModule::columnCount(const QModelIndex &/*parent*/) const
{
	return m_btoolMod==true?2:7;
}
QMimeData *taskModule::mimeData(const QModelIndexList &indexes) const
{
	QMimeData *mimeData = new QMimeData();
	QByteArray encodedData;

	QDataStream stream(&encodedData, QIODevice::WriteOnly);
	QSet<QString> set_names;
	foreach (const QModelIndex &index, indexes) {
		if (index.isValid()) {
			tag_titem * node = (tag_titem *)index.internalPointer();
			if (node)
			{
				if (node->vec_path.size()>0)
				{
					if (set_names.contains(node->vec_path[0])==false)
					{
						stream << QString::fromStdString(toJson(node->vec_path[0]).toStdString());
						set_names.insert(node->vec_path[0]);
					}
				}
			}
		}
	}
	mimeData->setData("application/vnd.text.list", encodedData);
	return mimeData;
}
QStringList taskModule::mimeTypes() const
{
	QStringList types;
	types << "application/vnd.text.list";
	return types;
}
QVariant taskModule::data(const QModelIndex &index, int role) const
{
	QVariant vret;
	if (!index.isValid())
		return vret;
	if (index.column()<0 || index.column()>=7)
		return vret;
	if (m_btoolMod==true && index.column()>1)
		return vret;
	if (role==Qt::DisplayRole || role==Qt::EditRole)
	{
		tag_titem * node = (tag_titem *)index.internalPointer();
		if (node)
		{
			if (node->vec_path.size()==1)
			{
				if (index.column()==0 && m_btoolMod==false)
					return pureName(node->vec_path.last());
				else if (index.column()==1 && m_btoolMod==false)
					return className(node->vec_path.last());
				else if (index.column()==1 && m_btoolMod==true)
					return pureName(node->vec_path.last());
				else if (index.column()==0 && m_btoolMod==true)
					return className(node->vec_path.last());

				return QVariant();

			}
			else if (node->vec_path.size()>1 && node->vec_path.size()<3)
			{
				if (index.column()==0)
					return internal_to_view(node->vec_path.last());
				if (index.column()>node->vec_path.size()-1)
					return QVariant();
				return internal_to_view(node->vec_path[index.column()]);
			}
			else if (node->vec_path.size()==3)
			{
				QVariant value =  m_mainBlock
						[node->vec_path[0]]
						[node->vec_path[1]]
						[node->vec_path[2]];
				QVariantMap paravs;
				if (value.type()==QVariant::Map)
					paravs = value.toMap();
				switch (index.column())
				{
				case 0:
					vret = internal_to_view(node->vec_path.last());
					break;
				case 1:
					vret = internal_to_view(node->vec_path[1]);
					break;
				case 2:
					if (paravs.contains("tooltip"))
						vret = internal_to_view(paravs["tooltip"]).toString();
					else
						vret =  internal_to_view(node->vec_path[index.column()]);
					break;
				case 3:
					if (paravs.contains("instance_value"))
						vret = internal_to_view(paravs["instance_value"]);
					else
						vret = internal_to_view(value);
					break;
				case 4:
					if (paravs.contains("type"))
						vret = internal_to_view(paravs["type"]);
					break;
				case 5:
					if (paravs.contains("default"))
						vret = internal_to_view(paravs["default"]);
					break;
				case 6:
					if (paravs.contains("range"))
						vret = internal_to_view(paravs["range"]);
					break;
				default:
					break;
				}
				return vret;
			}
		}
	}
	if (role==Qt::BackgroundRole)
	{
		tag_titem * node = (tag_titem *)index.internalPointer();
		if (node)
		{
			if (node->vec_path.size()==3 && index.column()==3)
			{
				QVariant value =  m_mainBlock
						[node->vec_path[0]]
						[node->vec_path[1]]
						[node->vec_path[2]];
				QVariantMap paravs;
				if (value.type()==QVariant::Map)
					paravs = value.toMap();
				if (paravs.contains("instance_value"))
				{
					unsigned int inst = paravs["instance_value"].toUInt();
					vret = QBrush(QColor(inst * 61%127+128 ,
								  inst * 121%127+128,
								  inst * 37%127+128));
				}

			}
		}
	}
	return vret;
}

bool taskModule::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (data(index, role) != value) {
		if (!index.isValid())
			return false;
		if (role==Qt::DisplayRole || role==Qt::EditRole)
		{
			tag_titem * node = (tag_titem *)index.internalPointer();
			if (node)
			{
				if(node->vec_path.size()==3)
				{
					QVariant cva =  m_mainBlock
							[node->vec_path[0]]
							[node->vec_path[1]]
							[node->vec_path[2]];
					QVariantMap paravs;
					if (cva.type()==QVariant::Map)
						paravs = cva.toMap();

					if (index.column()==3)
					{
						if (paravs.contains("instance_value"))
						{
							paravs["instance_value"] = view_to_internal( value ,
																		 m_mainBlock
																				 [node->vec_path[0]]
																				 [node->vec_path[1]]
																				 [node->vec_path[2]].toMap()["instance_value"].type()
																		 );
							m_mainBlock
									[node->vec_path[0]]
									[node->vec_path[1]]
									[node->vec_path[2]] = paravs;
							emit dataChanged(index, index, QVector<int>() << role);
							return true;
						}
						else if (node->vec_path[1]=="instance")
						{
							m_mainBlock
									[node->vec_path[0]]
									[node->vec_path[1]]
									[node->vec_path[2]] = view_to_internal( value ,
																			m_mainBlock
																					[node->vec_path[0]]
																					[node->vec_path[1]]
																					[node->vec_path[2]].type()
																			);
							emit dataChanged(index, index, QVector<int>() << role);
							return true;
						}
					}
				}
			}
		}

	}
	return false;
}

Qt::ItemFlags taskModule::flags(const QModelIndex &index) const
{
	Qt::ItemFlags f = Qt::NoItemFlags;
	if (!index.isValid())
		return f;

	tag_titem * node = (tag_titem *)index.internalPointer();
	if (node)
	{
		f = 	Qt::ItemIsSelectable |
				Qt::ItemIsEnabled |
				Qt::ItemIsDragEnabled;

		if(node->vec_path.size()==3)
		{
			QVariant cva =  m_mainBlock
					[node->vec_path[0]]
					[node->vec_path[1]]
					[node->vec_path[2]];
			QVariantMap paravs;
			if (cva.type()==QVariant::Map)
				paravs = cva.toMap();

			if (index.column()==3)
			{
				if (paravs.contains("instance_value")||node->vec_path[1]=="instance")
				{
					f |= Qt::ItemIsEditable ;
				}
			}
		}
	}
	return f;
}
//从JSON创建对象
bool taskModule::initFromJson(const QByteArray & json, const QString & path)
{
	bool bres = false;
	beginResetModel();
	bres = taskCell::initFromJson(json,path);
	const QStringList lst = this->function_names();
	endResetModel();
	return bres;
}
//清理
void taskModule::clear()
{
	beginResetModel();
	taskCell::clear();
	endResetModel();
}
unsigned int taskModule::set_function_instance(const QString & func, unsigned  int instance) //设置模块实例标识
{
	unsigned int res = 0;
	beginResetModel();
	res = taskCell::set_function_instance(func,instance);
	endResetModel();
	return res;
}
unsigned int taskModule::set_in_subject_instance(const QString & func,const QString & name, unsigned int instance) //设置输入接口实例标识
{
	unsigned int res = 0;
	beginResetModel();
	res = taskCell::set_in_subject_instance(func,name,instance);
	endResetModel();
	return res;

}
unsigned int taskModule::set_out_subject_instance(const QString & func,const QString & name, unsigned int instance) //设置输出接口实例标识
{
	unsigned int res = 0;
	beginResetModel();
	res = taskCell::set_out_subject_instance(func,name,instance);
	endResetModel();
	return res;
}

unsigned int taskModule::in_subject_instance(const QString & func, const QString & name) const
{
	int u = taskCell::in_subject_instance(func,name);
	m_pinInsValues.insert(u);
	return u;
}
unsigned int taskModule::out_subject_instance(const QString & func, const QString & name) const
{
	int u = taskCell::out_subject_instance(func,name);
	m_pinInsValues.insert(u);
	return u;
}
QModelIndex taskModule::paraIndex()
{
	int rows = rowCount();
	for (int i=0;i<rows;++i)
	{
		QModelIndex rootc = index(i,0);
		int subrows = rowCount(rootc);
		for (int j=0;j<subrows;++j)
		{
			QString s = data(index(j,0,rootc)).toString();
			if (s == "parameters")
				return index(j,0,rootc);
		}
	}
	return QModelIndex();
}
QVariant taskModule::set_parameters_instance(const QString & func,const QString & name, const QVariant & instance) //设置参数实例标识
{
	QVariant res ;
	beginResetModel();
	res = taskCell::set_parameters_instance(func,name,instance);
	endResetModel();
	return res;
}

void taskModule::set_additional_paras(const QString & func,const QMap<QString,QVariant> & p)
{
	beginResetModel();
	taskCell::set_additional_paras(func,p);
	endResetModel();
}
/*!
 * \brief taskModule::draw_direction
 * 模块的管脚的显示顺序是可以变化的。在json属性 additional_args 里，
 * IND,OUTD 存储两个数组。数组的元素决定了管脚出现在模块图示左侧还是右侧。
 * 更详细的说，小于等于0的数值会出现在左侧，大于0 的数值对应右侧。绝对值小的元素在下，
 * 绝对值大的元素在上。
 * The order in which the pins of the module are displayed can be changed.
 * In the JSON property Additional_args, IND,OUTD stores two arrays.
 * The elements of the array determine whether the pin appears to the left or
 *  right of the module diagram.In more detail, a value less than or equal to
 * 0 will appear on the left side, with a value greater than 0 corresponding
 * to the right. An element with a small absolute value at the bottom,
 * with a large absolute value on the element.
 * \param func		待设置的功能   The function to get
 * \param bInput	是否为输入管脚 true=Input, false = Output
 * \param n			管脚的编号（取决于自然顺序）。 Normal order index of the pin
 * \return 顺序值   Order Value
 * \see TGraphicsTaskItem::paint
 */
int taskModule::draw_direction(const QString & func, bool bInput, int n)  const
{
	if (m_mainBlock.contains(func) == false)
		return bInput==true?-1:1;
	if (m_mainBlock[func].contains("instance")==false)
		return bInput==true?-1:1;
	if (m_mainBlock[func]["instance"].contains("additional_args")==false)
		return bInput==true?-1:1;
	QString addArgs = m_mainBlock[func]["instance"]["additional_args"].toString();

	QMap<QString,QVariant> mp = string_to_map(addArgs);
	QString strMasks;
	if (bInput==true)
		strMasks = mp["IND"].toString();
	else
		strMasks = mp["OUTD"].toString();
	QStringList lstMasks = strMasks.split(",",QString::SkipEmptyParts);
	if (lstMasks.size()<=n)
		return bInput==true?-(n+1):(n+1);
	QString nn = lstMasks.at(n);
	return 	nn.toInt();
}

/*!
 * \brief taskModule::set_draw_direction
 *  * 模块的管脚的显示顺序是可以变化的。在json属性 additional_args 里，
 * IND,OUTD 存储两个数组。数组的元素决定了管脚出现在模块图示左侧还是右侧。
 * 更详细的说，小于等于0的数值会出现在左侧，大于0 的数值对应右侧。绝对值小的元素在下，
 * 绝对值大的元素在上。
 *  * The order in which the pins of the module are displayed can be changed.
 * In the JSON property Additional_args, IND,OUTD stores two arrays.
 * The elements of the array determine whether the pin appears to the left or
 *  right of the module diagram.In more detail, a value less than or equal to
 * 0 will appear on the left side, with a value greater than 0 corresponding
 * to the right. An element with a small absolute value at the bottom.
 * \param func 待设置的功能 The function to get
 * \param bInput 是否为输入管脚 true=Input, false = Output
 * \param n 管脚的编号（取决于自然顺序）。Normal order index of the pin
 * \param direction 新的顺序值  Order Value
 * \see TGraphicsTaskItem::paint
 */
void taskModule::set_draw_direction(const QString & func, bool bInput, int n, int direction)
{
	if (m_mainBlock.contains(func) == false)
		return ;
	if (m_mainBlock[func].contains("instance")==false)
		return ;
	QString addArgs = m_mainBlock[func]["instance"]["additional_args"].toString();

	QMap<QString,QVariant> mp = string_to_map(addArgs);
	QString strMasks;
	if (bInput==true)
		strMasks = mp["IND"].toString();
	else
		strMasks = mp["OUTD"].toString();
	QStringList lstMasks = strMasks.split(",",QString::SkipEmptyParts);
	QString strVal;
	const int sz = lstMasks.size();
	QString nstr = QString("%1").arg(direction);
	for (int i=0;i<=n || i<sz;++i)
	{
		if (i)
			strVal.push_back(",");
		if (i<sz)
		{
			if (i==n)
				strVal.push_back(nstr);
			else
				strVal.push_back(lstMasks.at(i));
		}
		else
		{
			if (i==n)
				strVal.push_back(nstr);
			else
				strVal.push_back(
							QString("%1").arg(
							bInput==true?-(i+1):(i+1)));
		}
	}
	if (bInput==true)
		mp["IND"] = strVal;
	else
		mp["OUTD"] = strVal;


	beginResetModel();
	m_mainBlock[func]["instance"]["additional_args"] = map_to_string(mp);
	endResetModel();
	return ;
}


