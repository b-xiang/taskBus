/*!
  * 本文件是类taskCell的实现文件。
  * taskBus中，一个独立的可执行文件称为一个模块。一个模块可以包含1个或多个功能。
  * taskCell存储这些功能的静态、动态信息。静态信息包括接口、参数表、功能本身的属性。
  * 动态信息包括运行时分配的参数值、实例名、输入输出连接关系。
  * This file is an implementation file for Class Taskcell. In Taskbus, a
  * standalone executable file is called a module. A module can consist of
  * 1 or more functions. Taskcell stores static, dynamic information for
  * these functions. Static information includes interfaces, parameter tables,
  * and properties of the function itself.
  * Dynamic information includes parameter values, instance names,
  * and input and output connection relationships that are assigned at run time.
  * @author goldenhawking@163.com, 2016-09
  */
#include "taskcell.h"
#include <QJsonObject>
#include <QDebug>
#include <QJsonParseError>
#include <QMimeData>
#include <QDataStream>
taskCell::taskCell()
{

}
taskCell::~taskCell()
{

}

/*!
 * \brief taskCell::initFromJson 从JSON创建对象 Creating objects from JSON
 * 会覆盖现有的功能信息，并追加新的功能信息。
 * Overwrites the existing feature information and appends the new feature
 * information.
 * \param json JSON string
 * \param path EXEC file path
 * \return When succeeded, return true.
 */
bool taskCell::initFromJson(const QByteArray & json, const QString & path)
{
	if (m_set_fullpaths.contains(path))
			return true;
	//根节点 Create Root node
	tag_titem * root_item = nullptr;
	if (m_idxs.size())
		root_item = m_idxs[0].get();
	else
	{
		root_item =
				new tag_titem {
				nullptr,
				QVector<tag_titem*>(),
				QVector<QString>()
		};
		m_idxs<<std::shared_ptr<tag_titem>(root_item);
	}

	try{
		QJsonParseError errormsg;
		QJsonDocument doc = QJsonDocument::fromJson(json ,&errormsg);
		if (doc.isNull()==true)
			throw QString("Broken Json format:")+errormsg.errorString();
		if (doc.isObject()==false)
			throw QString("Broken Json format:")+errormsg.errorString();

		QJsonObject objModules =  doc.object();
		QStringList functions = objModules.keys();
		//对每个功能名，进行操作 For each feature name, operate
		foreach (QString functionk, functions)
		{
			//0.....功能名称 Function Name
			const QString depth0_strFunctionName = functionk;
			//遍历子属性 Traversing child properties
			QJsonValue vCurFun = objModules.value(functionk);
			if (vCurFun.isObject()==false)
				continue;
			//插入树索引 Insert Tree Index
			tag_titem * item_0 = new tag_titem {
				root_item,
				QVector<tag_titem*>(),
				QVector<QString>()<<depth0_strFunctionName
			};
			root_item->children.push_back(item_0);
			m_idxs<<std::shared_ptr<tag_titem>(item_0);
			m_idxparas[item_0] = root_item->children.size()-1;


			QJsonObject objCurFun = vCurFun.toObject();
			//功能的属性 Properties of the function
			QStringList defkeys = objCurFun.keys();
			foreach (QString defk, defkeys)
			{
				//1.........类别名称 Category name
				const QString depth1_strClassName = defk;
				//插入树索引 Insert Tree Index
				tag_titem * item_1 = new tag_titem {
					item_0,
					QVector<tag_titem*>(),
					QVector<QString>()<<depth0_strFunctionName
									 <<depth1_strClassName
				};
				item_0->children.append(item_1);
				m_idxs<<std::shared_ptr<tag_titem>(item_1);
				m_idxparas[item_1] = item_0->children.size()-1;

				//遍历子属性 Traversing child properties
				QJsonValue vClass = objCurFun.value(defk);
				if (vClass.isObject()==false)
				{
					//2......属性名 Property name
					const QString depth2_strPropName = defk;
					const QVariant depth2_strPropValue = vClass.toVariant();
					m_mainBlock
							[depth0_strFunctionName]
							[depth1_strClassName]
							[depth2_strPropName] = depth2_strPropValue;
					//插入树索引 Insert Tree Index
					tag_titem * item_2 = new tag_titem {
						item_1,
						QVector<tag_titem*>(),
						QVector<QString>()<<depth0_strFunctionName
										 <<depth1_strClassName
										<<depth2_strPropName
					};
					item_1->children.append(item_2);
					m_idxs<<std::shared_ptr<tag_titem>(item_2);
					m_idxparas[item_2] = item_1->children.size()-1;
				}
				else
				{
					QJsonObject objCurProp = vClass.toObject();
					//功能的属性 Properties of the function
					QStringList propKeys = objCurProp.keys();
					foreach (QString propk, propKeys)
					{
						//2......属性名  Property name
						const QString depth2_strPropName = propk;
						//插入树索引  Insert Tree Index
						tag_titem * item_2 = new tag_titem {
							item_1,
							QVector<tag_titem*>(),
							QVector<QString>()<<depth0_strFunctionName
											 <<depth1_strClassName
											<<depth2_strPropName
						};
						item_1->children.append(item_2);
						m_idxs<<std::shared_ptr<tag_titem>(item_2);
						m_idxparas[item_2] = item_1->children.size()-1;

						const QJsonValue depth2_strPropValue
							  = objCurProp.value(propk);
						QVariantMap mp;


						if (depth2_strPropValue.isObject()==false)
						{
							m_mainBlock
									[depth0_strFunctionName]
									[depth1_strClassName]
									[depth2_strPropName]
									= depth2_strPropValue.toVariant();
						}
						else
						{
							mp = depth2_strPropValue.toObject().toVariantMap();
							if (mp.contains("instance_value") == false)
							{
								if (mp.contains("default")==true)
									mp["instance_value"] = mp["default"];
								else
									mp["instance_value"] = QString("");
							}
							m_mainBlock
									[depth0_strFunctionName]
									[depth1_strClassName]
									[depth2_strPropName] = mp;
								}

					}

				}
			}
			//为本功能添加运行时类别 Add a run-time category for this function
			if (m_mainBlock[depth0_strFunctionName].contains("instance")==false)
			{
				tag_titem * item_1_instance = new tag_titem {
					item_0,
					QVector<tag_titem*>(),
					QVector<QString>()<<depth0_strFunctionName<<"instance"
				};
				item_0->children.push_back(item_1_instance);
				m_idxs<<std::shared_ptr<tag_titem>(item_1_instance);
				m_idxparas[item_1_instance] = item_0->children.size()-1;

				tag_titem * item_2_instance = new tag_titem {
					item_1_instance,
					QVector<tag_titem*>(),
					QVector<QString>()<<depth0_strFunctionName
									 <<"instance"<<"instance_value"
				};
				item_1_instance->children.push_back(item_2_instance);
				m_idxs<<std::shared_ptr<tag_titem>(item_2_instance);
				m_idxparas[item_2_instance] =
					item_1_instance->children.size()-1;

				tag_titem * item_2_exec = new tag_titem {
					item_1_instance,
					QVector<tag_titem*>(),
					QVector<QString>()<<depth0_strFunctionName
									 <<"instance"<<"exec"
				};
				item_1_instance->children.push_back(item_2_exec);
				m_idxs<<std::shared_ptr<tag_titem>(item_2_exec);
				m_idxparas[item_2_exec] = item_1_instance->children.size()-1;

				tag_titem * item_2_additional_args = new tag_titem {
					item_1_instance,
					QVector<tag_titem*>(),
					QVector<QString>()<<depth0_strFunctionName
									 <<"instance"<<"additional_args"
				};
				item_1_instance->children.push_back(item_2_additional_args);
				m_idxs<<std::shared_ptr<tag_titem>(item_2_additional_args);
				m_idxparas[item_2_additional_args] =
					item_1_instance->children.size()-1;


				m_mainBlock[depth0_strFunctionName]["instance"]
					["instance_value"] = 0;
				m_mainBlock[depth0_strFunctionName]["instance"]
					["exec"] = path;
				m_mainBlock[depth0_strFunctionName]["instance"]
					["additional_args"] = "";
			}
		}
	}
	catch(QString message)
	{
		qDebug()<<message;
		return false;
	}
	m_set_fullpaths.insert(path);

	return true;
}

QByteArray taskCell::toJson(const QString & root)  const
{
	QMap<QString, QMap<QString, QMap<QString, QVariant> > > tarBlock;
	if (m_mainBlock.contains(root))
		tarBlock[root] = m_mainBlock[root];
	else
		tarBlock = m_mainBlock;
	QVariantMap map_m0;
	foreach (QString s0, tarBlock.keys())
	{
		QVariantMap map_m1;
		foreach (QString s1, tarBlock[s0].keys())
		{
			QVariantMap map_m2;
			foreach (QString s2, tarBlock[s0][s1].keys())
			{
				QVariant v = tarBlock[s0][s1][s2];
				map_m2[s2] = v;
			}
			map_m1[s1] = map_m2;
		}//endfor s1
		map_m0[s0] = map_m1;
	}//end for s0
	QJsonObject obj = QJsonObject::fromVariantMap(map_m0);
	QJsonDocument doc(obj);
	return doc.toJson(QJsonDocument::Indented);
}
void taskCell::clear()
{
	m_mainBlock.clear();
	m_idxparas.clear();
	m_idxs.clear();
}
unsigned  int taskCell::function_instance(const QString & func)  const
{
	if (m_mainBlock.contains(func) == false)
		return -1;
	if (m_mainBlock[func].contains("instance")==false)
		return -1;
	if (m_mainBlock[func]["instance"].contains("instance_value")==false)
		return -1;
	return 	m_mainBlock[func]["instance"]["instance_value"].toUInt();
}

QString taskCell::function_exec(const QString & func ) const
{
	if (m_mainBlock.contains(func) == false)
		return "";
	if (m_mainBlock[func].contains("instance")==false)
		return "";
	if (m_mainBlock[func]["instance"].contains("exec")==false)
		return "";
	return 	m_mainBlock[func]["instance"]["exec"].toString();
}

QString taskCell::set_function_exec(const QString & func , const QString & execstr )
{
	QString old ;
	if (m_mainBlock.contains(func) == false)
		return old;
	if (m_mainBlock[func].contains("instance")==false)
		return old;
	old = m_mainBlock[func]["instance"]["exec"].toString();
	m_mainBlock[func]["instance"]["exec"] = execstr;
	return old;
}
const QString taskCell::function_class(const QString & func) const
{
	QStringList strV = func.split(QRegExp("[:_]"),QString::SkipEmptyParts);
	if (strV.size()>1)
	{
		strV.pop_back();
		QString res;
		foreach(QString v, strV)
		{
			if (res.size())
				res += ":";
			res += v;
		}
		return res;
	}
	return 	QObject::tr("Global");
}
QString taskCell::function_tooltip(const QString & func ) const
{
	if (m_mainBlock.contains(func) == false)
		return func;
	if (m_mainBlock[func].contains("name")==false)
		return func;
	if (m_mainBlock[func]["name"].contains("name")==false)
		return func;
	return 	m_mainBlock[func]["name"]["name"].toString();
}
unsigned int taskCell::set_function_instance(const QString & func,
											 unsigned int instance)
{
	unsigned int oldins = 0;
	if (m_mainBlock.contains(func) == false)
		return -1;
	if (m_mainBlock[func].contains("instance")==false)
		return -1;
	m_mainBlock[func]["instance"]["instance_value"] = instance;
	return oldins;
}

//功能的所有接口
const QStringList taskCell::in_subjects(const QString & func) const
{
	QStringList lst;
	if (m_mainBlock.contains(func) == false)
		return lst;
	if (m_mainBlock[func].contains("input_subjects")==false)
		return lst;
	return m_mainBlock[func]["input_subjects"].keys();
}
const QStringList taskCell::out_subjects(const QString & func)  const
{
	QStringList lst;
	if (m_mainBlock.contains(func) == false)
		return lst;
	if (m_mainBlock[func].contains("output_subjects")==false)
		return lst;
	return m_mainBlock[func]["output_subjects"].keys();
}
const QVariantMap taskCell::in_subject(const QString & func,
									   const QString & name)  const
{
	QVariantMap mp;
	if (m_mainBlock.contains(func) == false)
		return mp;
	if (m_mainBlock[func].contains("input_subjects")==false)
		return mp;
	if (m_mainBlock[func]["input_subjects"].contains(name)==false)
		return mp;
	if ( m_mainBlock[func]["input_subjects"][name].type()!=QVariant::Map)
		return mp;
	mp = m_mainBlock[func]["input_subjects"][name].toMap();
	return mp;
}
const QVariantMap taskCell::out_subject(const QString & func,
										const QString & name)  const
{
	QVariantMap mp;
	if (m_mainBlock.contains(func) == false)
		return mp;
	if (m_mainBlock[func].contains("output_subjects")==false)
		return mp;
	if (m_mainBlock[func]["output_subjects"].contains(name)==false)
		return mp;
	if ( m_mainBlock[func]["output_subjects"][name].type()!=QVariant::Map)
		return mp;
	mp = m_mainBlock[func]["output_subjects"][name].toMap();
	return mp;
}
unsigned int taskCell::in_subject_instance(const QString & func,
										   const QString & name)  const
{
	unsigned int ins = 0;
	if (m_mainBlock.contains(func) == false)
		return ins;
	if (m_mainBlock[func].contains("input_subjects")==false)
		return ins;
	if (m_mainBlock[func]["input_subjects"].contains(name)==false)
		return ins;
	if ( m_mainBlock[func]["input_subjects"][name].type()!=QVariant::Map)
		return ins;
	QVariantMap	mp = m_mainBlock[func]["input_subjects"][name].toMap();
	if (mp.contains("instance_value")==false)
		return ins;
	return mp["instance_value"].toUInt();
}
unsigned int taskCell::out_subject_instance(const QString & func,
											const QString & name)  const
{
	unsigned int ins = 0;
	if (m_mainBlock.contains(func) == false)
		return ins;
	if (m_mainBlock[func].contains("output_subjects")==false)
		return ins;
	if (m_mainBlock[func]["output_subjects"].contains(name)==false)
		return ins;
	if ( m_mainBlock[func]["output_subjects"][name].type()!=QVariant::Map)
		return ins;
	QVariantMap	mp = m_mainBlock[func]["output_subjects"][name].toMap();
	if (mp.contains("instance_value")==false)
		return ins;
	return mp["instance_value"].toUInt();
}
QString taskCell::in_subject_tooltip(const QString & func,
									 const QString & name)  const
{
	if (m_mainBlock.contains(func) == false)
		return name;
	if (m_mainBlock[func].contains("input_subjects")==false)
		return name;
	if (m_mainBlock[func]["input_subjects"].contains(name)==false)
		return name;
	if ( m_mainBlock[func]["input_subjects"][name].type()!=QVariant::Map)
		return name;
	QVariantMap	mp = m_mainBlock[func]["input_subjects"][name].toMap();
	if (mp.contains("tooltip")==false)
		return name;
	return mp["tooltip"].toString();
}
QString taskCell::out_subject_tooltip(const QString & func,
									  const QString & name)  const
{
	if (m_mainBlock.contains(func) == false)
		return name;
	if (m_mainBlock[func].contains("output_subjects")==false)
		return name;
	if (m_mainBlock[func]["output_subjects"].contains(name)==false)
		return name;
	if ( m_mainBlock[func]["output_subjects"][name].type()!=QVariant::Map)
		return name;
	QVariantMap	mp = m_mainBlock[func]["output_subjects"][name].toMap();
	if (mp.contains("tooltip")==false)
		return name;
	return mp["tooltip"].toString();
}

unsigned  int taskCell::set_in_subject_instance(const QString & func,
												const QString & name,
												unsigned int instance)
{
	unsigned int ins = 0;
	if (m_mainBlock.contains(func) == false)
		return ins;
	if (m_mainBlock[func].contains("input_subjects")==false)
		return ins;
	if (m_mainBlock[func]["input_subjects"].contains(name)==false)
		return ins;
	if ( m_mainBlock[func]["input_subjects"][name].type()!=QVariant::Map)
		return ins;
	QVariantMap	mp = m_mainBlock[func]["input_subjects"][name].toMap();
	unsigned int oldins = mp["instance_value"] .toUInt();
	mp["instance_value"] = instance;
	m_mainBlock[func]["input_subjects"][name] = mp;
	return oldins;
}
unsigned int taskCell::set_out_subject_instance(const QString & func,
												const QString & name,
												unsigned int instance)
{
	unsigned int ins = 0;
	if (m_mainBlock.contains(func) == false)
		return ins;
	if (m_mainBlock[func].contains("output_subjects")==false)
		return ins;
	if (m_mainBlock[func]["output_subjects"].contains(name)==false)
		return ins;
	if ( m_mainBlock[func]["output_subjects"][name].type()!=QVariant::Map)
		return ins;
	QVariantMap	mp = m_mainBlock[func]["output_subjects"][name].toMap();
	unsigned int oldins = mp["instance_value"] .toUInt();
	mp["instance_value"] = instance;
	m_mainBlock[func]["output_subjects"][name] = mp;
	return oldins;
}//设置输出接口实例标识


QString taskCell::in_subject_item(const QString & func,
								  const QString & name,
								  const QString & key)  const
{
	if (m_mainBlock.contains(func) == false)
		return "";
	if (m_mainBlock[func].contains("input_subjects")==false)
		return "";
	if (m_mainBlock[func]["input_subjects"].contains(name)==false)
		return "";
	if ( m_mainBlock[func]["input_subjects"][name].type()!=QVariant::Map)
		return "";
	QVariantMap	mp = m_mainBlock[func]["input_subjects"][name].toMap();
	if (mp.contains(key)==false)
		return "";
	return mp[key].toString();
}
QString taskCell::out_subject_item(const QString & func,
								   const QString & name,
								   const QString & key)  const
{
	if (m_mainBlock.contains(func) == false)
		return "";
	if (m_mainBlock[func].contains("output_subjects")==false)
		return "";
	if (m_mainBlock[func]["output_subjects"].contains(name)==false)
		return "";
	if ( m_mainBlock[func]["output_subjects"][name].type()!=QVariant::Map)
		return "";
	QVariantMap	mp = m_mainBlock[func]["output_subjects"][name].toMap();
	if (mp.contains(key)==false)
		return "";
	return mp[key].toString();
}

const QStringList  taskCell::parameters(const QString & func) const
{
	QStringList lst;
	if (m_mainBlock.contains(func) == false)
		return lst;
	if (m_mainBlock[func].contains("parameters")==false)
		return lst;
	return m_mainBlock[func]["parameters"].keys();
}
const QVariantMap  taskCell::parameter(const QString & func,
									   const QString & name)  const
{
	QVariantMap mp;
	if (m_mainBlock.contains(func) == false)
		return mp;
	if (m_mainBlock[func].contains("parameters")==false)
		return mp;
	if (m_mainBlock[func]["parameters"].contains(name)==false)
		return mp;
	if ( m_mainBlock[func]["parameters"][name].type()!=QVariant::Map)
		return mp;
	mp = m_mainBlock[func]["parameters"][name].toMap();
	return mp;
}
QVariant  taskCell::parameters_instance(const QString & func,
										const QString & name)  const
{
	QVariant ins;
	if (m_mainBlock.contains(func) == false)
		return ins;
	if (m_mainBlock[func].contains("parameters")==false)
		return ins;
	if (m_mainBlock[func]["parameters"].contains(name)==false)
		return ins;
	if ( m_mainBlock[func]["parameters"][name].type()!=QVariant::Map)
		return ins;
	QVariantMap	mp = m_mainBlock[func]["parameters"][name].toMap();
	if (mp.contains("instance_value")==false)
		return ins;
	return mp["instance_value"];
}
QVariant taskCell::set_parameters_instance(const QString & func,
										   const QString & name,
										   const QVariant & instance)
{
	QVariant ins;
	if (m_mainBlock.contains(func) == false)
		return ins;
	if (m_mainBlock[func].contains("parameters")==false)
		return ins;
	if (m_mainBlock[func]["parameters"].contains(name)==false)
		return ins;
	if ( m_mainBlock[func]["parameters"][name].type()!=QVariant::Map)
		return ins;
	QVariantMap	mp = m_mainBlock[func]["parameters"][name].toMap();
	QVariant oldins = mp["instance_value"];
	mp["instance_value"] = instance;
	m_mainBlock[func]["parameters"][name] = mp;
	return oldins;
}
/*!
 * \brief taskCell::additional_paras 获取扩展属性
 * taskBus支持未来通过additional_paras扩展功能。目前，已经在其中扩展了进程优先级、
 * GUI布局等功能。
 * 这些选项也是使用键值存储的。不同的是，他们存储在一个独立的字符串中，
 * 使用Key=Value的形式。各项键值之间使用分号分隔。
 * 比如： “nice=2;IND=1,2,3,4;OUTD=-1,-2,-3,-5;”
 * Taskbus supports future extended functionality through Additional_paras.
 * At present, the process priority, GUI layout and other functions have been
 * expanded. These options are also stored using key values.
 * The difference is that they are stored in a separate string, using the form
 * of key=value. Separate the key values with semicolons.
 * For example: "NICE=2;IND=1,2,3,4;OUTD=-1,-2,-3,-5;"
 * \param func 待获取的功能名 Function Name
 * \return  map
 */
QMap<QString,QVariant> taskCell::additional_paras(const QString & func) const
{
	QMap<QString,QVariant> mp;
	if (m_mainBlock.contains(func) == false)
		return mp;
	if (m_mainBlock[func].contains("instance")==false)
		return mp;
	if (m_mainBlock[func]["instance"].contains("additional_args")==false)
		return mp;
	QString addArgs = m_mainBlock[func]["instance"]
			["additional_args"].toString();

	mp = string_to_map(addArgs);
	return mp;
}

/*!
 * \brief taskCell::set_additional_paras 设置扩展属性
 * \see taskCell::additional_paras
 * \see taskCell::map_to_string
 */
void taskCell::set_additional_paras(const QString & func,
									const QMap<QString,QVariant> & p)
{
	if (m_mainBlock.contains(func) == false)
		return ;
	if (m_mainBlock[func].contains("instance")==false)
		return ;
	QString addArgs = m_mainBlock[func]["instance"]
			["additional_args"].toString();

	QMap<QString,QVariant> mp = string_to_map(addArgs);
	QStringList keys = 	p.keys();
	foreach(QString k, keys)
	{
		mp[k] = p[k];
	}
	m_mainBlock[func]["instance"]["additional_args"] =
			map_to_string(mp);

}

/*!
 * \brief taskCell::internal_to_view 从内部存储格式到友好显示格式的转换。
 * 由于界面上显示复杂数据结构是不方便的，友好格式会用逗号进行分割显示。
 * Conversions from the internal storage format to the friendly display format.
 * Because it is inconvenient to display complex data structures on the
 * interface, the friendly format is separated by commas.
 * \param vtdef 待转换的变量 Variables to convert
 * \return 转换后的变量 The converted Variable
 */
QVariant taskCell::internal_to_view(const QVariant & vtdef)
{
	QVariant vret;
	if (vtdef.type()==QVariant::Map)
	{
		QJsonObject obj = QJsonObject::fromVariantMap(vtdef.toMap());
		QJsonDocument doc(obj);
		vret = doc.toJson(QJsonDocument::Compact).toStdString().c_str();
	}
	else if (vtdef.type()==QVariant::List)
	{
		QString strVal;
		QVariantList lstv = vtdef.toList();
		foreach(QVariant vii, lstv)
		{
			if (strVal.size())
				strVal+=",";
			strVal+=vii.toString();
		}
		vret = strVal;
	}
	else
		vret = vtdef;
	return vret;
}

/*!
 * \brief taskCell::view_to_internal 从友好显示格式到内部存储格式的转换。
 * Conversions from the friendly display format to the internal storage format .
 * \param vtdef 待转换的变量 Variables to convert
 * \param targetType 转换后的变量 The converted Variable
 * \return
 */
QVariant taskCell::view_to_internal(const QVariant & vtdef
									, QVariant::Type targetType)
{
	QVariant vret;
	if (targetType==QVariant::Map)
	{
		QJsonDocument doc = QJsonDocument::fromJson(vtdef.toByteArray());
		QJsonObject obj = doc.object();
		vret = obj.toVariantMap();
	}
	else if (targetType==QVariant::List)
	{
		QString strVal = vtdef.toString();
		QStringList lstv = strVal.split(",",QString::SkipEmptyParts);
		QVariantList lsttag;
		foreach(QString vii, lstv)
		{
			lsttag << vii;
		}
		vret = strVal;
	}
	else
		vret = vtdef;
	return vret;
}
/*!
 * \brief taskCell::pureName 返回简单的功能名称。taskBus的功能名称前面可以加入分类。
 * 带有分类信息的功能名称类似“Class_Func”，有助于在GUI界面实现分类筛选。这个函数仅返
 * 回“Func”部分。
 * Returns a simple feature name. Module Function names in Taskbus can take a
 * category part before the pure function name. For example, "Class_func",
 * which helps to implement classification filtering in the GUI interface.
 * This function returns only the "Func" section.
 * \param v 完整名称 full name with class names
 * \return  简单名称 pure function name
 */
QString taskCell::pureName(const QVariant &  v)
{
	QString sv = v.toString();
	QStringList ls = sv.split(QRegExp("[:_]"),QString::SkipEmptyParts);
	if (ls.size())
		return ls.last();
	return QString("");
}
/*!
 * \brief taskCell::className 返回功能类名称。taskBus的功能名称前面可以加入分类。
 * 带有分类信息的功能名称类似“Class_Func”，有助于在GUI界面实现分类筛选。这个函数仅返
 * 回“Func”部分。
 * Returns the class name. Module Function names in Taskbus can take a
 * category part before the pure function name. For example, "Class_func",
 * which helps to implement classification filtering in the GUI interface.
 * This function returns only the "Class" section.
 * \param v 完整名称 full name with class names
 * \return  类名称 class name
 */
QString taskCell::className(const QVariant & v)
{
	QString sv = v.toString();
	QStringList ls = sv.split(QRegExp("[:_]"),QString::SkipEmptyParts);
	QString res;
	while (ls.size()>1)
	{
		if (res.size())
			res+=":";
		res += ls.first();
		ls.pop_front();
	}
	return res;
}

/*!
 * \brief taskCell::map_to_string 提供由Map向键值字符串的转换。
 * Provides a conversion from a map to a key-value string.
 * \param m 键值 key-value
 * \return 字符串 string
 */
QString taskCell::map_to_string(const QMap<QString, QVariant> & m)
{
	QString s;
	for(QMap<QString, QVariant>::const_iterator p = m.begin();p!=m.end();++p)
	{
		s += p.key();
		s += "=";
		s += p.value().toString();
		s += ";";
	}
	return(s);
}
/*!
 * \brief taskCell::string_to_map 提供由键值字符串向Map的转换。
 *  Provides a conversion from a  key-value string to a map.
 * \param s 字符串 string
 * \return 键值 key-value
 */
QMap<QString, QVariant> taskCell::string_to_map(const QString & s)
{
	QMap<QString, QVariant> res;
	QStringList lst = s.split(";");
	foreach (QString s, lst)
	{
		int t = s.indexOf("=");
		if (t>0 && t< s.size())
		{
			QString name = s.left(t).trimmed();
			QString value = s.mid(t+1).trimmed();
			res[name] = value;
		}
	}
	return(res);
}
