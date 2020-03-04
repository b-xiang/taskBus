/*!
  * 本文件是类taskCell的头文件。taskCell存储模块的描述信息。它既包含模块JSON文件中预
  * 先定义的输入输出接口、参数配置项，又包括运行时配置的参数。
  * This file is the header file for class Taskcell. It contains description
  * information for the Taskcell enclosure. It contains both pre-defined input
  * and output interfaces in the module's JSON file, and the parameters that
  * are configured at run time.
  * @author goldenhawking@163.com, 2016-09
  */
#ifndef TASKCELL_H
#define TASKCELL_H
#include <QMap>
#include <QVariant>
#include <QVector>
#include <memory>
#include <QString>
#include <QSet>
#include <QJsonDocument>
/*!
 * \brief The taskCell class
 * 用于存储和Json相关的基本数据结构的类
 * Classes for storing and JSON-related basic data structures
 * 这个数据结构描述了模块的静态、动态信息。可以脱离图形界面进行操作。
 * This data structure describes the static and dynamic information of the
 * module.
 */
class taskCell
{
protected:
	/*!
	 * \brief The tag_titem struct
	 * 该结构体用于维护树形数据结构，并为图形界面中的派生QAbstractItemModel提供基
	 * 础数据结构。
	 * The structure is used to maintain the tree data structure and provide
	 * the underlying data structure for the derived Qabstractitemmodel in the
	 * graphical interface.
	 */
	struct tag_titem{
		tag_titem * parent;						//父节点
		QVector<tag_titem *> children;			//子节点
		QVector<QString> vec_path;				//JSON路径
	};

public:
	taskCell();
	virtual ~taskCell();
	const QSet<QString>  & full_paths() const {return m_set_fullpaths;}
protected:
	/*!
	 * \brief m_mainBlock主结构：功能键、大类、属性键、属性值
	 * Main structure: function key, category key, property key, attribute value
	 */
	QMap<QString, QMap<QString, QMap<QString, QVariant> > > m_mainBlock;
	/*!
	 * \brief m_idxs 树形数据结构  tree data structure
	 */
	QVector<std::shared_ptr<tag_titem> > m_idxs;
	/*!
	 * \brief m_idxparas 节点位于同层兄弟节点的顺序。用于为QItemModel模型提供行号。
	 * The node's row order in current layer .
	 * Used to provide line numbers for the Qitemmodel model.
	 */
	QMap<const tag_titem*, int > m_idxparas;
	//路径，防止重复读取
	QSet<QString> m_set_fullpaths;
public:
	//工具函数 toolkit funtions
	static QString pureName(const QVariant & v) ;
	static QString className(const QVariant & v) ;
	static QVariant internal_to_view(const QVariant & v)  ;
	static QVariant view_to_internal(const QVariant & v,
									 QVariant::Type targetType) ;
	static 	QString map_to_string(const QMap<QString, QVariant> & m);
	static  QMap<QString, QVariant> string_to_map(const QString & s);
public:
	//JSON对象转化 Creating objects from JSON and convert it to JSON
	virtual bool initFromJson(const QByteArray & json, const QString & path = "");
	virtual QByteArray toJson(const QString & root) const;
	//清理 Cleaning
	virtual void clear();
	//获取主属性数据结构 Get the main attribute data structure
	const QMap<QString, QMap<QString, QMap<QString, QVariant> > > & block()
	const { return  m_mainBlock;}

	/*!
	  *模块功能属性读写
	  * Module function Properties read/write
	  */
public:
	//获取模块名 Get Module Names
	virtual const QStringList function_names() const {return block().keys();}
	//获取首个模块的名字 Get the name of the first module
	virtual const QString function_firstname() const
	{
		return block().keys().empty()?QString():block().keys().first();
	}
	virtual const QString function_class(const QString & func) const;
	//获取模块实例标识 Get the module instance ID
	virtual unsigned int function_instance(const QString & func)  const ;
	//设置模块实例标识 Set the module instance ID
	virtual unsigned int set_function_instance(const QString & func
											   , unsigned  int instance) ;
	//获取模块功能的友好信息 Get friendly information on module functions
	virtual QString function_tooltip(const QString & func ) const;
	//获取模块功能的可执行文件路径 Get the executable file path for module
	virtual QString function_exec(const QString & func ) const;
	//设置模块功能的可执行文件路径 Set the executable file path for module
	virtual QString set_function_exec(const QString & func
									  , const QString & execstr);
	/*!
	  *接口属性读写
	  * Module interface Properties read/write
	  */
public:
	//获取所有输入接口名 Get all input interface names
	virtual const QStringList in_subjects(const QString & func) const ;
	//获取所有输出接口名 Get all output interface names
	virtual const QStringList out_subjects(const QString & func)  const ;
	//获取输入接口的所有信息 Get all the information for the input interface
	virtual const QVariantMap in_subject(const QString & func,
										 const QString & name)  const ;
	//获取输出接口的所有信息 Get all the information for the output interface
	virtual const QVariantMap out_subject(const QString & func,
										  const QString & name)  const ;
	//获取输入接口实例标识 Get the input interface instance ID
	virtual unsigned int in_subject_instance(const QString & func,
											 const QString & name)  const ;
	//获取输出接口实例标识 Get the output interface instance ID
	virtual unsigned int out_subject_instance(const QString & func,
											  const QString & name)  const ;
	//获取输入接口友好信息 Get the input interface friendly information
	virtual QString in_subject_tooltip(const QString & func,
									   const QString & name)  const ;
	//获取输出接口友好信息 Get the output interface friendly information
	virtual QString out_subject_tooltip(const QString & func,
										const QString & name)  const ;
	//获取输入接口属性值
	virtual QString in_subject_item(const QString & func,const QString & name,
									const QString & key)  const ;
	//获取输出接口属性值
	virtual QString out_subject_item(const QString & func,const QString & name,
									 const QString & key)  const ;
	//设置输入接口实例标识 Setting the input interface instance ID
	virtual unsigned int set_in_subject_instance(const QString & func,
												 const QString & name,
												 unsigned int instance);
	//设置输出接口实例标识 Setting the output interface instance ID
	virtual unsigned int set_out_subject_instance(const QString & func,
												  const QString & name,
												  unsigned int instance);
	/*!
	  *静态参数属性读写
	  * Module static paraments Properties read/write
	  */
public:
	//获取所有参数名 Get all parameter names
	virtual const QStringList parameters(const QString & func) const ;
	//获取参数的所有信息 Get all information about a parameter
	virtual const QVariantMap parameter(const QString & func,
										const QString & name)  const ;
	//获取参数实例标识 Get parameter Instance ID
	virtual QVariant parameters_instance(const QString & func,
										 const QString & name)  const ;
	//设置参数实例标识 Set parameter Instance ID
	virtual QVariant set_parameters_instance(const QString & func,
											 const QString & name,
											 const QVariant & instance);

	//获取额外参数 Get extra parameters
	virtual QMap<QString,QVariant> additional_paras(const QString & func) const;
	//设置额外参数 Setting Additional parameters
	virtual void set_additional_paras(const QString & func,
									  const QMap<QString,QVariant> & p);
};

#endif // TASKCELL_H
