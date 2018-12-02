/*! 本文件定义了工程类的接口。
  * 工程类用于最终管理各个模块，并在他们之间传递数据。
  * This document defines the interface of the engineering class.
  * Engineering classes are used to ultimately manage each module and pass data
  *  between them.
  * @author goldenhawking@163.com, 2016-09
  */
#ifndef TASKPROJECT_H
#define TASKPROJECT_H
#include <QByteArray>
#include <QMap>
#include <QSet>
#include <QThread>
#include <QVector>
#include <QObject>
#include <QVariant>
#include <functional>
class taskCell;
class taskNode;
/*!
 * \brief The taskProject class 本类承载了一个工程的所有基本功能（除了GUI设计）
 * 1.taskProject 提供了一个场景，用来盛放功能模块以及他们之间的链接关系。
 * 2.taskProject 提供一个统一的操作方法，用来启动各个模块、周转数据。
 * 3.taskProject 具备外部接口功能，自身可以作为一个独立的功能模块嵌入其他taskProject
 * 场景中（嵌套）。
 * 这些特性是通过一系列的数据结构完成的。本类即定义了这些数据结构，以及维护方法。
 * This class carries all the basic functions of a project (except GUI design)
 * 1.taskProject provides a scene to hold function modules and link
 * relationships between them.
 * 2.taskProject provides a unified operation method to start the various
 *  modules, turnover data.
 * 3.taskProject with external interface function, can be used as a separate
 * function module embedded in other Taskproject In its scene (nested).
 * These features are accomplished through a series of data structures.
 * This class defines these data structures, as well as the maintenance methods.
 */
class taskProject : public QObject
{
	Q_OBJECT
public:
	taskProject(QObject * parent = NULL);
	virtual ~taskProject();
public:
	//向场景添加实例 Add an function instance to a scene
	void			add_node		(const QString json,QPointF pt,
									 bool rebuildidx = true);
	//删除实例(给定序号) Delete instance (given ordinal)
	void			del_node		(int);
	//转换为JSON Convert to JSON
	QByteArray		toJson			();
	//从JSON创建 Create from JSON
	void			fromJson		(QByteArray json, taskCell * m_pRefModel);
	//获得某个模块的命令行 To obtain a command line for a module
	QString			cmdlinePrg		(taskCell * cell);
	//获得某个模块的命令行参数 Get command-line arguments for a module
	QStringList		cmdlineParas	(taskCell * cell);//生成参数列表
	//设置某个外部管脚（专题）的值 Set the value of an external pin
	void			set_outside_id_in (QString name, unsigned int ins);
	void			set_outside_id_out (QString name, unsigned int ins);

	bool isRunning(){return m_bRunning;}

public slots:
	//重新计算并更新连接 Recalculate and update connections
	void refresh_idxes();
	void start_project();
	void stop_project();
	void stop_project(QThread * pIdleThread);

	//外部发送数据 Send data externally
	void slot_outside_recieved(QByteArray arr);

	//基本成员访问方法 Basic member access methods
public:
	//获得功能管理器taskNode实例列表
	//Get the Feature Manager Tasknode instance list
	const QVector< taskNode * > & vec_nodes(){return m_vec_nodes;}
	//获得功能管理器taskCell实例列表
	//Get the Feature Manager Taskcell instance list
	const QVector< taskCell *> & vec_cells(){return m_vec_cells;}
	//获得内部索引。从实例ID到实例列表下标
	//Gets the internal index. Subscript from Instance ID to instance list
	const QMap< unsigned int, int> & idx_instance2vec(){return m_idx_instance2vec;}
	//获得内部索引。从taskNode实例指针到实例列表下标
	//Gets the internal index. Subscript from the Tasknode instance pointer to the instance list
	const QMap< taskNode * , int> & idx_node2vec(){return m_idx_node2vec;}
	//获得内部索引。输入专题ID : 对该ID感兴趣的所有实例列表下标
	//Gets the internal index. Input subject ID : Valid Consumers' instance list
	const QMap< unsigned int, QVector<unsigned int> > & idx_in2instances(){return m_idx_in2instances;}
	//获得内部索引。输入专题ID : 专题名
	//Gets the internal index. Input feature ID: Subject name
	const QMap< unsigned int, QVector<QString> > & idx_in2names(){return m_idx_in2names;}
	//获得内部索引。输出专题ID : 能够生产该类专题的所有实例列表下标
	//Gets the internal index. Output subject ID : Valid Producers' instance list
	const QMap< unsigned int, QVector<unsigned int> > & idx_out2instances(){return m_idx_out2instances;}
	//获得内部索引。输出专题ID : 专题名
	//Gets the internal index. Output feature ID: Subject name
	const QMap< unsigned int, QVector<QString> >  & idx_out2names(){return m_idx_out2names;}

	/*!外部属性与索引
	 * 对外索引，整个工程可以作为一个单独的模块运行
	 * 悬空的管脚（专题），作为模块整体的输入输出.管脚（专题）的取值为随机内部值
	 * External properties and indexes
	 * the entire project can be run as a separate module
	 * The dangling pin, as the input and output of the module as a whole.
	 * PIN values are random internal values
	 */
public:
	//外部输入管脚（专题）名称->外部输入管脚（专题）ID External input pin name-> External input PIN number
	const QMap< QString, unsigned int> & map_outside_name2id_in(){return m_outside_name2id_in;}
	//外部输出管脚（专题）名称->外部输出管脚（专题）ID External output pin name-> External output PIN number
	const QMap< QString, unsigned int> & map_outside_name2id_out(){return m_outside_name2id_out;}
	//内部悬空管脚（专题）临时ID->实例ID Internal dangling pin temporary id-> instance ID
	const QMap< unsigned int, unsigned int> & idxout_hang_in2instance(){return m_hang_in2instance;}
	const QMap< unsigned int, unsigned int> & idxout_hang_out2instance(){return m_hang_out2instance;}
	//内部悬空管脚（专题）临时ID->内部功能名称 Internal dangling pin temporary id-> internal function name
	const QMap< unsigned int, QString> & idxout_hang_in2name(){return m_hang_in2name;}
	const QMap< unsigned int, QString> & idxout_hang_out2name(){return m_hang_out2name;}
	//内部悬空管脚（专题）临时ID->外部功能名称 Internal dangling pin temporary id-> external function name
	const QMap< unsigned int, QString> & idxout_hang_in2fullname(){return m_hang_in2fullname;}
	const QMap< unsigned int, QString> & idxout_hang_out2fullname(){return m_hang_out2fullname;}
	//外部功能名称->内部悬空管脚（专题）临时ID External feature name-> Internal dangling pin temporary ID
	const QMap< QString, unsigned int> & idxout_hang_fullname2in(){return m_hang_fullname2in;}
	const QMap< QString ,unsigned int> & idxout_hang_fullname2out(){return m_hang_fullname2out;}
	//外部映射表 External mapping table
	//内部悬空管脚（专题）临时ID->外部管脚（专题）ID Internal dangling pin temporary id -> external pin ID
	const QMap< unsigned int, unsigned int> & idxout_iface_inside2outside_in(){return m_iface_inside2outside_in;}
	const QMap< unsigned int, unsigned int> & idxout_iface_inside2outside_out(){return m_iface_inside2outside_out;}
	//外部管脚（专题）ID->内部悬空管脚（专题）临时ID external pin ID -> Internal dangling pin temporary id
	const QMap< unsigned int, unsigned int> & idxout_iface_outside2inside_in(){return m_iface_outside2inside_in;}
	const QMap< unsigned int, unsigned int> & idxout_iface_outside2inside_out(){return m_iface_outside2inside_out;}
	//调试模式，保存输入各个模块的原始数据、各个模块输出的所有数据
	//Debug mode, save the raw data of the input modules,
	// and all the data of each module output
	void turnOnDebug(int nidx);
	void turnOffDebug(int nidx);
	//进程优先级 Process priority
	int  get_nice(int) const;
	int  set_nice(int,int);

	bool isWarpperPrj() const {return m_bWarpper;}
	void setWarpperPrj(bool b){m_bWarpper = b; refresh_idxes();}

signals:
	void sig_message(QString str);
	void sig_cmd_start(QObject * node, QString cmd, QStringList paras);
	void sig_cmd_stop(QObject * node);
	void sig_cmd_write(QObject * node, QByteArray arr);
	//外部通信 External communication
	void sig_outside_new_package(QByteArray);
	void sig_started();
	void sig_stopped();
private slots:
	void slot_new_package(QByteArray);
	void slot_new_command(QMap<QString,QVariant> cmd);
	void slot_new_errmsg(QByteArray);
	void slot_pro_started();
	void slot_pro_stopped();


	//一些为了GUI程序提供的回调接口
	//Some callback interfaces provided for GUI programs
public:
	//创建cell的工厂。由于cell类可作为model的基类，所以提供创建对象的回调
	//Create a factory for the cell. Because the cell class can be used as
	// the base class for the model, the callback that
	// creates the object is provided
	void setCallback_NewCell(std::function<taskCell * (void)> fun){ m_fNewCell = fun;}
	//当一个实例cell被添加到本项目后，会回调这个函数
	//When an instance cell is added to this project, the function is called back
	void setCallback_InsAppended(std::function<void (taskCell * pmod, taskNode * pnod,QPointF pt)> fun){ m_fInsAppended = fun;}
	//当索引被建立后，会回调这个函数
	//When the index is established, the function is called back
	void setCallback_IndexRefreshed(std::function<void (void)> fun){ m_fIndexRefreshed = fun;}
	//取得某个模块的位置。图形界面应该实现这个功能。
	//Gets the location of a module. The graphical interface should implement this function.
	void setCallback_GetCellPos(std::function<QPointF (int)> fun){ m_fGetCellPos = fun;}

private:
	taskCell *	default_NewCell();
	void		default_InsAppended(taskCell * pmod, taskNode * pnod,QPointF pt);
	void		default_Indexrefreshed();
	QPointF		default_GetCellPos(int);

	/*!
	  *关键成员变量
	  * Key members
	  */
private:
	/*!
	 * \brief m_bWarpper 当前Project是不是作为整体嵌入其他Project中。
	 * Is project currently embedded in other project as a whole?
	 */
	bool m_bWarpper = false;
	/*!
	 * \brief m_bRunning 当前工程是否正在运行。
	 * Whether the current project is running.
	 */
	bool m_bRunning = false;
	/*!
	 * \brief instance_count 静态成员。用于为新插入的模块分配实例ID。这个ID是局部
	 * 的，作用区域不会超过本类实例。
	 * Static members. Used to assign the instance ID to the newly
	 * inserted module. This ID is local and does not exceed the scope of this
	 * class instance.
	 */
	static int instance_count;
	//线程池大小 Thread pool Size
	const int m_nthreads = 6;
	//功能管理器taskNode实例列表
	//the Feature Manager Tasknode instance list
	QVector< taskNode * > m_vec_nodes;
	//功能管理器taskCell实例列表
	//the Feature Manager Taskcell instance list
	QVector< taskCell *> m_vec_cells;
	//负责吞吐的线程池；The thread pool responsible for data transmission;
	QMap<QThread *, QSet<taskNode *> > m_map_threadpool;
	//外部输入管脚（专题）名称->外部输入管脚（专题）ID External input pin name-> External input PIN number
	QMap< QString, unsigned int> m_outside_name2id_in;
	//外部输出管脚（专题）名称->外部输出管脚（专题）ID External output pin name-> External output PIN number
	QMap< QString, unsigned int> m_outside_name2id_out;

	/*!
	  * 索引成员变量，会在refresh_idxes调用时生成
	  * Index member variables that is generated when refresh_idxes is called
	  */
private:
	QMap< unsigned int, int> m_idx_instance2vec;
	QMap< taskNode * , int> m_idx_node2vec;
	QMap< unsigned int, QVector<unsigned int> > m_idx_in2instances;
	QMap< unsigned int, QVector<QString> > m_idx_in2names;
	QMap< unsigned int, QVector<unsigned int> > m_idx_out2instances;
	QMap< unsigned int, QVector<QString> > m_idx_out2names;
private:
	QMap< unsigned int, unsigned int> m_hang_in2instance;
	QMap< unsigned int, QString> m_hang_in2name;
	QMap< unsigned int, QString> m_hang_in2fullname;
	QMap< QString, unsigned int> m_hang_fullname2in;
	QMap< unsigned int, unsigned int> m_hang_out2instance;
	QMap< unsigned int, QString> m_hang_out2name;
	QMap< unsigned int, QString> m_hang_out2fullname;
	QMap< QString ,unsigned int> m_hang_fullname2out;
	QMap< unsigned int, unsigned int> m_iface_inside2outside_in;
	QMap< unsigned int, unsigned int> m_iface_inside2outside_out;
	QMap< unsigned int, unsigned int> m_iface_outside2inside_in;
	QMap< unsigned int, unsigned int> m_iface_outside2inside_out;
private:
	std::function<taskCell * (void)> m_fNewCell;
	std::function<void (taskCell * pmod, taskNode * pnod,QPointF pt)> m_fInsAppended;
	std::function<void (void)> m_fIndexRefreshed;
	std::function<QPointF (int)> m_fGetCellPos;

};

#endif // TASKPROJECT_H
