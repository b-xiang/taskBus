/*!
  * taskProject 类的实现。
  * @author goldenhawking@163.com, 2016-09
  */
#include "taskproject.h"
#include "tasknode.h"
#include "taskcell.h"
#include "tb_interface.h"
#include <QCoreApplication>
#include <QJsonObject>
#include <QPointF>
#include <QDebug>
#include <QSettings>
#include <QTimerEvent>
#include <algorithm>
#include "process_prctl.h"
#include "../watchdog/profile_log.h"
QSet<quint32> taskProject::m_instance_set;
taskProject::taskProject(QObject * parent ):
	QObject(parent),
	m_fNewCell(std::bind(&taskProject::default_NewCell,this)),
	m_fInsAppended(std::bind(&taskProject::default_InsAppended,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3)),
	m_fIndexRefreshed(std::bind(&taskProject::default_Indexrefreshed,this)),
	m_fGetCellPos(std::bind(&taskProject::default_GetCellPos,this,std::placeholders::_1))
{
	//初始化线程池。
	for(int i=0;i<m_nthreads;++i)
	{
		QThread * pt = new QThread(this);
		pt->start();
		m_map_threadpool[pt].size();
	}

	m_nTimerID = startTimer(200);

}
taskProject::~taskProject()
{
	stop_project();
	QList<QThread *> lstThreads = m_map_threadpool.keys();
	foreach (QThread * k, lstThreads)
	{
		k->terminate();
		k->wait();
	}
	foreach (QThread * k, lstThreads)
		k->deleteLater();
	foreach (taskNode * n, m_vec_nodes)
		n->deleteLater();
	foreach (taskCell * n, m_vec_cells)
	{
		//由于在图形界面里，cell可能会派生出QObject的多重继承。因此，这里要判断。
		//Because in the graphical interface, the cell may derive qobject
		//multiple inheritance. So, here to judge.
		QObject * obj = dynamic_cast<QObject *> (n);
		if (!obj)
			delete n;
		else if (obj->parent()==nullptr)
			obj->deleteLater();
	}
}

quint32	taskProject::new_instance()
{
	quint32 nextIns = 1;

	while (m_instance_set.contains(nextIns))
		++nextIns;
	m_instance_set.insert(nextIns);

	return nextIns;
}
void	taskProject::unreg_instance(const quint32 ins)
{
	m_instance_set.remove(ins);
}

/*!
 * \brief taskProject::append_instance 向本工程添加一个功能
 * Add a component to the project.
 * \param json 功能描述 Function description
 * \param pt 构件初始的图形显示位置 The initial graphics position of the component
 * \param rebuildidxs 是否重建索引 Whether to rebuild the index
 */
void taskProject::add_node(const QString json,QPointF pt,bool rebuildidx)
{
	taskNode * node = new taskNode(0);
	taskCell * mod = m_fNewCell();
	mod->initFromJson(QByteArray(json.toStdString().c_str()));
	if (!mod->function_names().size())
		node->deleteLater();
	else
	{
		node->bindCell(mod);
		QString vname = mod->function_names().first();
		mod->set_function_instance(vname,new_instance());
		QMap<QString, QVariant> mp_nice = mod->additional_paras(vname);
		//进程优先级默认值为“TASKBUS::pnice_norm”，一般优先级
		//Process priority default value is "Taskbus::p nice_norm",
		//general priority
		if (mp_nice.contains("nice")==false)
			mp_nice["nice"] = TASKBUS::pnice_norm;
		mod->set_additional_paras(vname,mp_nice);
		m_vec_nodes.push_back(node);
		m_vec_cells.push_back(mod);

		//从线程池捡一个最空闲的线程承载新的节点通信。
		//Pick one of the most idle threads from the thread pool to host
		//the new node communication
		QThread * bestTh = 0;
		int minLoad = 0x7fffffff;
		QList<QThread *> lstThreads = m_map_threadpool.keys();
		foreach(QThread *pth,lstThreads)
		{
			if (minLoad>m_map_threadpool[pth].size())
			{
				minLoad = m_map_threadpool[pth].size();
				bestTh = pth;
			}
		}
		Q_ASSERT(bestTh);
		node->moveToThread(bestTh);
		m_map_threadpool[bestTh].insert(node);

		//关联各个事件 Associating individual events
		connect (node, &taskNode::sig_new_package, this, &taskProject::slot_new_package,Qt::QueuedConnection);
		connect (node, &taskNode::sig_new_command, this, &taskProject::slot_new_command,Qt::QueuedConnection);
		connect (node, &taskNode::sig_new_errmsg, this, &taskProject::slot_new_msg,Qt::QueuedConnection);
		connect (node, &taskNode::sig_pro_started, this, &taskProject::slot_pro_started,Qt::QueuedConnection);
		connect (node, &taskNode::sig_pro_stopped, this, &taskProject::slot_pro_stopped,Qt::QueuedConnection);
		connect (this, &taskProject::sig_cmd_start, node, &taskNode::cmd_start,Qt::QueuedConnection);
		connect (this, &taskProject::sig_cmd_stop, node, &taskNode::cmd_stop,Qt::QueuedConnection);
		connect (this, &taskProject::sig_cmd_write, node, &taskNode::cmd_write,Qt::QueuedConnection);
		connect (this, &taskProject::sig_cmd_sendcmd, node, &taskNode::cmd_sendcmd,Qt::QueuedConnection);
		connect (node, &taskNode::sig_iostat, this, &taskProject::sig_iostat);

		//回调额外的索引创建事件 Callback for additional index creation events
		m_fInsAppended(mod,node,pt);
		if (rebuildidx)
			this->refresh_idxes();
	}
}

void taskProject::del_node(int node)
{
	int sz = m_vec_nodes.size();
	if (node>=0 && node < sz)
	{
		foreach (QThread * t, m_map_threadpool.keys())
			m_map_threadpool[t].remove(m_vec_nodes[node]);
		const QString func = m_vec_cells[node]->function_firstname();
		unsigned int instanceid =  m_vec_cells[node]->function_instance(func);
		unreg_instance(instanceid);
		//清理节点
		m_vec_nodes[node]->deleteLater();
		if (node>=0)
		{
			QObject * obj = dynamic_cast<QObject *> (m_vec_cells[node]);
			if (!obj)
				delete m_vec_cells[node];
			else
				obj->deleteLater();
		}
		m_vec_nodes.remove(node);
		m_vec_cells.remove(node);
	}
	else
	{
		for (int i=0;i<sz;++i)
		{
			foreach (QThread * t, m_map_threadpool.keys())
				m_map_threadpool[t].remove(m_vec_nodes[i]);
			const QString func = m_vec_cells[i]->function_firstname();
			unsigned int instanceid =  m_vec_cells[i]->function_instance(func);
			unreg_instance(instanceid);
			//清理节点
			m_vec_nodes[i]->deleteLater();
			if (node>=0)
			{
				QObject * obj = dynamic_cast<QObject *> (m_vec_cells[i]);
				if (!obj)
					delete m_vec_cells[i];
				else
					obj->deleteLater();
			}
		}
		m_vec_nodes.clear();
		m_vec_cells.clear();
	}
	this->refresh_idxes();
}

/*!
 * \brief taskProject::set_outside_id_in 仅在本Project作为子项目时有用
 * 用来设置外部的专题（管脚）ID
 * Useful only when this project is a subproject.
 * Used to set the external Subject (PIN) ID
 * \param name 悬空管脚（专题）名 Subject Name
 * \param ins  悬空管脚（专题）ID Subject ID
 */
void taskProject::set_outside_id_in (QString name, unsigned int ins)
{
	if (idxout_hang_fullname2in().keys().contains(name))
	{
		m_outside_name2id_in[name] = ins;
		refresh_idxes();
	}
}
/*!
 * \brief taskProject::set_outside_id_out 仅在本Project作为子项目时有用
 * 用来设置外部的专题（管脚）ID
 * Useful only when this project is a subproject.
 * Used to set the external Subject (PIN) ID
 * \param name 悬空管脚（专题）名 Subject Name
 * \param ins  悬空管脚（专题）ID Subject ID
 */
void taskProject::set_outside_id_out (QString name, unsigned int ins)
{
	if (idxout_hang_fullname2out().keys().contains(name))
	{
		m_outside_name2id_out[name] = ins;
		refresh_idxes();
	}

}

/*!
 * \brief taskProject::refresh_idxes 根据各个专题的ID，构件索引
 * ID Widget index based on each topic
 */
void taskProject::refresh_idxes()
{
	LOG_PROFILE("CORE","rebuild indexes.");
	m_idx_instance2vec.clear();
	m_idx_in2instances.clear();
	m_idx_in2names.clear();
	m_idx_out2instances.clear();
	m_idx_out2names.clear();
	m_idx_node2vec.clear();

	QSet<unsigned int> used_ids;
	int sizeV = m_vec_nodes.size();
	//for each nodes , do
	for (int i=0;i<sizeV;++i)
	{
		taskNode * node = m_vec_nodes[i];
		taskCell * model = m_vec_cells[i];
		m_idx_node2vec[node] = i;
		//only care about the first function
		QStringList lstfuns = model->function_names();
		if (lstfuns.size()!=1)
			continue;
		QString func = lstfuns.first();
		unsigned int instanceid = model->function_instance(func);
		//record map from instanceid to order (i)
		m_idx_instance2vec[instanceid] = i;
		//in subjects
		QStringList lstSrcSubs = model->in_subjects(func);
		foreach (QString bname,lstSrcSubs )
		{
			unsigned int subid = model->in_subject_instance(func,bname);
			if (subid)
			{
				m_idx_in2instances[subid].push_back(instanceid);
				m_idx_in2names[subid].push_back(bname);
				used_ids.insert(subid);
			}
		}
		//out subjects
		QStringList lstOutSubs = model->out_subjects(func);
		foreach (QString bname,lstOutSubs )
		{
			unsigned int subid = model->out_subject_instance(func,bname);
			if (subid)
			{
				m_idx_out2instances[subid].push_back(instanceid);
				m_idx_out2names[subid].push_back(bname);
				used_ids.insert(subid);
			}
		}

	}

	//悬空管脚索引创建。悬空管脚作为整体工程对外的接口。
	//The dangling pin index will be created. The dangling pin is an
	//external interface for the whole project.
	m_hang_in2instance.clear();
	m_hang_in2name.clear();
	m_hang_in2fullname.clear();
	m_hang_fullname2in.clear();
	m_hang_fullname2out.clear();
	m_hang_out2instance.clear();
	m_hang_out2name.clear();
	m_hang_out2fullname.clear();
	if (m_bWarpper)
	{
		unsigned int start_test_id= 1;
		for (int i=0;i<sizeV;++i)
		{
			taskCell * model = m_vec_cells[i];

			QStringList lstfuns = model->function_names();
			if (lstfuns.size()!=1)
				continue;
			QString func = lstfuns.first();
			unsigned int instanceid = model->function_instance(func);
			//in
			QStringList lstSrcSubs = model->in_subjects(func);
			foreach (QString bname,lstSrcSubs )
			{
				unsigned int subid = model->in_subject_instance(func,bname);
				if (!subid)
				{
					while (used_ids.contains(start_test_id))
						++start_test_id;
					used_ids.insert(start_test_id);
					m_hang_in2instance[start_test_id] = instanceid;
					m_hang_in2name[start_test_id] = bname;
					QString fulln =
							m_hang_in2fullname[start_test_id] = func+"_"+ bname
							+ QString("i%1").arg(instanceid);
					m_hang_fullname2in[ fulln] = start_test_id;
				}
			}
			//out
			QStringList lstOutSubs = model->out_subjects(func);
			foreach (QString bname,lstOutSubs )
			{
				unsigned int subid = model->out_subject_instance(func,bname);
				if (!subid)
				{
					while (used_ids.contains(start_test_id))
						++start_test_id;
					used_ids.insert(start_test_id);
					m_hang_out2instance[start_test_id] = instanceid;
					m_hang_out2name[start_test_id] = bname;
					QString fulln =
							m_hang_out2fullname[start_test_id] = func+"_"+ bname
							+ QString("o%1").arg(instanceid);
					m_hang_fullname2out[ fulln] = start_test_id;
				}
			}
		}

	}
	//内外映射 internal and external mapping
	m_iface_inside2outside_in.clear();
	m_iface_inside2outside_out.clear();
	m_iface_outside2inside_in.clear();
	m_iface_outside2inside_out.clear();
	if (m_bWarpper)
	{
		QList<unsigned int> inside_ins_ins = m_hang_in2fullname.keys();
		foreach (unsigned int in, inside_ins_ins)
		{
			const QString  fullname = m_hang_in2fullname[in];
			if (m_outside_name2id_in.contains(fullname))
			{
				m_iface_inside2outside_in[in] = m_outside_name2id_in[fullname];
				m_iface_outside2inside_in[m_outside_name2id_in[fullname]] = in;
			}
		}

		QList<unsigned int> inside_ins_outs = m_hang_out2fullname.keys();
		foreach (unsigned int out, inside_ins_outs)
		{
			const QString  fullname = m_hang_out2fullname[out];
			if (m_outside_name2id_out.contains(fullname))
			{
				m_iface_inside2outside_out[out] = m_outside_name2id_out[fullname];
				m_iface_outside2inside_out[m_outside_name2id_out[fullname]] = out;
			}
		}

	}
	//回调函数，用来通知图形界面刷新。
	//callback function to notify the graphical interface to refresh.
	m_fIndexRefreshed();
	LOG_PROFILE("CORE","rebuild indexes finished.");
}
/*!
 * \brief taskProject::toJson conver to JSON
 * \return  JSON string in UTF-8
 */
QByteArray taskProject::toJson()
{
	QJsonObject obj_root;
	obj_root["total_mods"] = m_vec_cells.size();
	int nModels = m_vec_cells.size();
	for (int nmodidx = 0;nmodidx<nModels;++nmodidx)
	{
		taskCell * tmod = m_vec_cells[nmodidx];
		//模型本身
		QString key = QString("mod%1").arg(nmodidx);
		QJsonDocument doc = QJsonDocument::fromJson( tmod->toJson(""));
		obj_root[key] = doc.object();
		//位置
		QPointF pf = m_fGetCellPos(nmodidx);
		QJsonObject posobj;
		posobj["x"] = pf.x();
		posobj["y"] = pf.y();
		key = QString("pos%1").arg(nmodidx);
		obj_root[key] = posobj ;
	}

	QJsonDocument doc(obj_root);
	return doc.toJson(QJsonDocument::Indented);
}

/*!
 * \brief taskProject::fromJson Fill project from json
 * \param json  JSON project ,from file *.tbj
 * \param m_pRefModel 场景中可用的模块列表。
 * A list of modules available in the scene.
 */
void taskProject::fromJson(QByteArray json, taskCell * m_prefCell)
{
	QJsonDocument doc = QJsonDocument::fromJson(json);
	QJsonObject obj_root = doc.object();
	if (obj_root.isEmpty())
		return;
	int insts = obj_root["total_mods"].toInt();
	for(int i=0;i<insts;++i)
	{
		QString key = QString("mod%1").arg(i);
		QJsonObject obj_cur = obj_root[key].toObject();
		if (obj_cur.isEmpty())
			continue;
		QJsonDocument doc_st (obj_cur);
		QByteArray arr_cur = doc_st.toJson();
		//参照本机模块的路径，来覆盖工程中的路径。在不同的计算机上，模块的绝对路径是不同的。
		//Refer to the path of the native module to overwrite the path in the
		//project. On different computers, the absolute path of the module is different.
		taskCell md;
		md.initFromJson(arr_cur);
		if (md.function_names().size()==0)
			continue;
		QString funcName = md.function_names().first();
		if (m_prefCell->function_names().contains(funcName)==false)
		{
			qDebug()<<tr("Mod %1 does not exists.").arg(funcName);
			continue;
		}
		//纠正路径 Correct path
		QString insPath = m_prefCell->function_exec(funcName);
		md.set_function_exec(funcName,insPath);

		key = QString("pos%1").arg(i);
		double fx = obj_root[key].toObject()["x"].toDouble();
		double fy = obj_root[key].toObject()["y"].toDouble();
		add_node(md.toJson(funcName),QPointF(fx,fy),false);
	}
	refresh_idxes();

}

QString  taskProject::cmdlinePrg(taskCell * model)
{
	QString cmdline;
	const QStringList functions = model->function_names();
	if (functions.empty())
		return cmdline;
	const QString func = functions.first();
	unsigned int func_ins = model->function_instance(func);
	if (func_ins==0)
		return cmdline;
	return model->function_exec(func);
}

QStringList taskProject::cmdlineParas(taskCell * model)
{
	QStringList lst;
	const QStringList functions = model->function_names();
	if (functions.empty())
		return lst;
	const QString func = functions.first();
	unsigned int func_ins = model->function_instance(func);
	if (func_ins==0)
		return lst;
	//参数，传入进程实例ID parameters, incoming process instance ID
	lst<<QString("--instance=%1").arg(func_ins);
	lst<<QString("--function=%1").arg(func);

	//后续参数，传入各个paras
	//Subsequent parameters, passing in each paras
	const QStringList paranames = model->parameters(func);
	foreach (const QString paraname,paranames)
	{
		const QVariant v = model->parameters_instance(func,paraname);
		if (v.isValid())
		{
			switch(v.type())
			{
			case QVariant::Double:
			{
				lst<<QString("--%1=%2").arg(paraname).arg(model->internal_to_view(v).toDouble(),0,'g',15);
				break;
			}
			default:
				lst<<QString("--%1=%2").arg(paraname).arg(model->internal_to_view(v).toString());
				break;
			}

		}
	}
	//传入各个输入输出管脚（专题）的instance
	//Incoming instance for each input/output pin (special subject)
	const QStringList in_subs = model->in_subjects(func);
	foreach (const QString subname,in_subs)
	{
		const unsigned int v = model->in_subject_instance(func,subname);
		QString fulln =  func+"_"+ subname
				+ QString("i%1").arg(func_ins);
		if (v>0)
			lst<<QString("--%1=%2").arg(subname).arg(v);
		else if (m_hang_fullname2in.contains(fulln))
			lst<<QString("--%1=%2").arg(subname).arg(m_hang_fullname2in[fulln]);
	}
	const QStringList out_subs = model->out_subjects(func);
	foreach (const QString subname,out_subs)
	{
		const unsigned int v = model->out_subject_instance(func,subname);
		QString fulln =  func+"_"+ subname
				+ QString("o%1").arg(func_ins);
		if (v>0)
			lst<<QString("--%1=%2").arg(subname).arg(v);
		else if (m_hang_fullname2out.contains(fulln))
			lst<<QString("--%1=%2").arg(subname).arg(m_hang_fullname2out[fulln]);
	}
	return  lst;
}

/*!
 * \brief PDesignerView::slot_new_package 数据包收转 Packet transfer
 * \param pkg 一个完整的数据包 A complete packet
 * \see taskNode::slot_readyReadStandardOutput
 */
void taskProject::slot_new_package(QByteArrayList pkgs)
{
	LOG_PROFILE("IO","Start Dealing packs.");
	foreach (QByteArray pkg, pkgs)
	{
		if (static_cast<size_t>(pkg.size())<sizeof(TASKBUS::subject_package_header))
			continue;
		const TASKBUS::subject_package_header * header =
				reinterpret_cast<const TASKBUS::subject_package_header *>(pkg.constData());
		if (header->data_length+sizeof( TASKBUS::subject_package_header)!=static_cast<size_t>(pkg.size()))
			continue;
		bool blocked = false;
		try {
			taskNode * pNodeSrc = qobject_cast<taskNode*>(sender());
			if (!pNodeSrc)
				throw "pNodeSrc is not sender() type";
			if (m_idx_node2vec.contains(pNodeSrc)==false)
				throw "node idx does not contain sender";
			int nvecidx = m_idx_node2vec[pNodeSrc];
			taskCell * mod = m_vec_cells[nvecidx];
			QStringList srcfuns = mod->function_names();
			if (srcfuns.size()==0)
				throw "srcfuns.size() is 0";
			//看看是否合法 See if it's legal.
			if (header->subject_id!=0xffffffff)
			{
				quint32 src_inst = mod->function_instance(srcfuns.first());
				//专题是否被登记为内部专题
				//Whether the subject was registered as an internal topic
				if (m_idx_out2instances.contains(header->subject_id)==true)
				{
					//是否为本专题合法生产者
					//Whether it is a legitimate producer of this topic
					if (m_idx_out2instances[header->subject_id].contains(src_inst)==false)
						throw tr("source instance %1 is not a valid producer for subject %2.")
							.arg(src_inst)
							.arg(header->subject_id);

					//专题是否被登记
					//Whether the subject has registered consumers
					if (m_idx_in2instances.contains(header->subject_id)==false)
						throw tr("input subject %1 has no valid recievers.")
							.arg(header->subject_id);
					//是否为合法头部 legal head
					if (!(header->prefix[0]!=0x3C || header->prefix[1]!=0x5A || header->prefix[2]!=0x7E || header->prefix[3]!=0x69))
					{
						//传输给所有消费者 Transfer to all consumers
						foreach (quint32 uins, m_idx_in2instances[header->subject_id] )
						{
							if (m_idx_instance2vec.contains(uins)==false)
								continue;
							int bidx = m_idx_instance2vec[uins];
							emit sig_cmd_write(m_vec_nodes[bidx],pkg);
							//有接收者阻塞了 There's a receiver blocking it.
							if (m_vec_nodes[bidx]->outputQueueSize()>16)
								blocked = true;
						}
						//防止过度缓存耗尽内存。
						//Prevents excessive cache depletion of memory.
						pNodeSrc->setBlockFlag(blocked);

					}
				}
				//专题是否被登记为外部专题
				//Whether the topic was registered as an external topic
				else if (m_hang_out2instance.contains(header->subject_id)==true && m_bWarpper==true)
				{
					//是否为本专题合法生产者
					//Whether it is a legitimate producer of this topic
					if (m_hang_out2instance[header->subject_id]==src_inst)
					{
						//专题名字获取 Topic name Acquisition
						QString strFullName = m_hang_out2fullname[header->subject_id];

						//专题是否需要向外发射 Whether the topic needs to be launched outward
						if (m_outside_name2id_out.contains(strFullName)==true&&
								!(header->prefix[0]!=0x3C || header->prefix[1]!=0x5A || header->prefix[2]!=0x7E || header->prefix[3]!=0x69))
						{
							TASKBUS::subject_package_header * wrheader =
									reinterpret_cast<TASKBUS::subject_package_header *>(pkg.data());
							wrheader->subject_id = m_outside_name2id_out[strFullName];
							//发送 Transmit
							emit sig_outside_new_package(pkg);
						}

					}
				}
				else
					throw tr("output subject \"%1\" does not exits.").arg(header->subject_id);

			}
			else
			{
				//指令处理 Instruction processing
				//! It will be dealed in each nodes' thread.
				Q_ASSERT(false);
			}
		}
		catch(QString msg)
		{
			push_msg(msg);
		}
		catch(const char * msg)
		{
			push_msg(msg);
		}

		if (blocked)
			push_msg(QString("Blocked by later process."));
	}
	LOG_PROFILE("IO","End Dealing packs.");
}
/*!
 * \brief taskProject::slot_new_command commands is another way to handover messages.
 * \param cmd message.
 * \details
 * taskbus支持跨模块的消息。消息由分号分割的键-值序列组成。以下三个保留的键名包括：
 * 1.source 来源模块的标识。标识的值由模块自身定义并在手册中声明。建议采用UUID或者全域名。
 * 2.destin 目的模块的标识。可以有多组。ALL表示向所有模块发送。
 * 3.function 功能名。
 * 其他参数任意指定。
 * taskBus support messages over modules. messages is a set of key-value pairs,
 * which is splitted by ";". Key-value are connected with "=".
 * There are 3 reserved keywords:
 * 1.source: ID of source module. ID is given by each module designer, UUID or
 * full domain name is strongly recommanded.
 * 2.destin: ID of destin modules. "all" means all modules. module names are seperated by ","
 * 3.function: function name.
 *
 * eg:
 * soucre=fft.ghstudio.org;destin=detector.clip.wav,plots.3dshow;function=spec_append;size=1024;
 * means:
 * source is from  fft.ghstudio.org
 * destin is to detector.clip.wav and plots.3dshow
 * function is spec_append
 * other parameters: size=1024
 */
void taskProject::slot_new_command(QMap<QString,QVariant> cmd)
{
	if (cmd.size())
	{
		QString source;
		if (cmd.contains("source"))
			source = cmd["source"].toString().trimmed();
		else
			return;
		if (cmd.contains("destin"))
		{
			const QStringList destins = cmd["destin"].toString().split(",");
			QSet<QString> notified;
			foreach (QString destinstr, destins)
			{
				const QString des = destinstr.trimmed();
				if (des!=source)
					notified.insert(des);
			}
			emit sig_cmd_sendcmd(cmd,notified);
		}
		else
			return;
	}
}

/*!
 * \brief PDesignerView::slot_new_errmsg 新的输出日志 New output Log
 * \param msg 日志体 Log body
 */
void taskProject::slot_new_msg(QByteArrayList msgs)
{
	taskNode * pNodeSrc = qobject_cast<taskNode*>(sender());

	if (!pNodeSrc)
	{
		send_msg(QString("Project:"),msgs);
		return;
	}
	if (m_idx_node2vec.contains(pNodeSrc)==false)
	{
		send_msg(QString("Node%1:").arg((quint64)pNodeSrc),msgs);
		return;
	}
	int nvecidx = m_idx_node2vec[pNodeSrc];
	taskCell * mod = m_vec_cells[nvecidx];
	QStringList srcfuns = mod->function_names();
	if (srcfuns.size()==0)
	{
		send_msg(QString("Node%1:").arg((quint64)pNodeSrc),msgs);
		return;
	}
	quint32 srcins = mod->function_instance(srcfuns.first());
	QString smsg = srcfuns.first() + QString("(%1):").arg(srcins);
	send_msg(smsg,msgs);
}

/*!
 * \brief PDesignerView::slot_pro_started 进程已经启动
 * Process has started
 */
void taskProject::slot_pro_started()
{
	taskNode * pNodeSrc = qobject_cast<taskNode*>(sender());
	if (!pNodeSrc)
		return;
	if (m_idx_node2vec.contains(pNodeSrc)==false)
	{
		return;
	}
	int nvecidx = m_idx_node2vec[pNodeSrc];
	taskCell * mod = m_vec_cells[nvecidx];
	QStringList srcfuns = mod->function_names();
	if (srcfuns.size()==0)
		return;
	quint32 srcins = mod->function_instance(srcfuns.first());
	QString smsg = srcfuns.first() + QString("(%1):").arg(srcins);
	smsg += "Started.";
	push_msg(smsg);
}
void taskProject:: slot_pro_stopped()
{
	taskNode * pNodeSrc = qobject_cast<taskNode*>(sender());
	if (!pNodeSrc)
		return;
	if (m_idx_node2vec.contains(pNodeSrc)==false)
	{
		return;
	}
	int nvecidx = m_idx_node2vec[pNodeSrc];
	taskCell * mod = m_vec_cells[nvecidx];
	QStringList srcfuns = mod->function_names();
	if (srcfuns.size()==0)
		return;
	quint32 srcins = mod->function_instance(srcfuns.first());
	QString smsg = srcfuns.first() + QString("(%1):").arg(srcins);
	smsg += "Stopped.";
	push_msg(smsg);
}

void taskProject::start_project()
{
	QSettings settings(QCoreApplication::applicationFilePath()+".ini",QSettings::IniFormat);
	int pr = settings.value("settings/nice",TASKBUS::pnice_norm).toInt();
	TASKBUS::set_proc_nice(pr);
	settings.setValue("settings/nice",pr);

	if (m_bRunning)
		return;

	int ns = m_vec_nodes.size();
	for (int i=0;i<ns;++i)
	{
		QString cmd = this->cmdlinePrg(m_vec_cells[i]);
		QStringList lstp = this->cmdlineParas(this->m_vec_cells[i]);
		emit sig_cmd_start(m_vec_nodes[i],cmd,lstp);
	}
	m_bRunning = true;
	emit sig_started();
}
/*!
 * \brief taskProject::stop_project
 * GUI app please use void taskProject::stop_project(QThread * pIdelThread)
 */
void taskProject::stop_project()
{
	if (m_bRunning==false)
		return;
	int ns = m_vec_nodes.size();
	for (int i=0;i<ns;++i)
	{
		emit sig_cmd_stop(m_vec_nodes[i]);
	}
	int nrun = 0;
	do{
		 nrun = 0;
		for (int i=0;i<m_vec_nodes.size();++i)
			nrun += m_vec_nodes[i]->isRunning()?1:0;
		QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
		QThread::msleep(50);
	}while (nrun);

	TASKBUS::set_proc_nice(TASKBUS::pnice_norm);
	emit sig_stopped();
	m_bRunning = false;
}
/*!
 * \brief taskProject::stop_project
 * \param pIdelThread GUI thread for GUI apps.
 * 为了保证密集的IO不会阻塞界面，每个工程的IO吞吐是在独立的线程运行的。
 * 由于工程本身又和GUI相关，因此在工程启动时，事件会被“转移（moveToThread）”
 * 到独立线程；工程结束运行，则重新回到GUI线程。
 * To ensure that dense IO does not block the interface, IO throughput for
 * each project runs on a separate thread. Because the project itself is related
 *  to the GUI, when the project starts, the event is "transferred (Movetothread)"
 * to the stand-alone thread, and when the project is finished running,
 * it returns to the GUI thread.
 */
void taskProject::stop_project(QThread * pIdelThread)
{
	if (m_bRunning==false)
		return;
	int ns = m_vec_nodes.size();
	for (int i=0;i<ns;++i)
	{
		emit sig_cmd_stop(m_vec_nodes[i]);
	}
	int nrun = 0;
	do{
		 nrun = 0;
		for (int i=0;i<m_vec_nodes.size();++i)
			nrun += m_vec_nodes[i]->isRunning()?1:0;
		QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
		QThread::msleep(50);
	}while (nrun);

	TASKBUS::set_proc_nice(TASKBUS::pnice_norm);
	moveToThread(pIdelThread);

	emit sig_stopped();
	m_bRunning = false;
}
/*!
 * \brief taskProject::default_NewCell 默认的模块工厂。
 * 注意，在图形化界面里，taskCell会派生出支持图形化设计的子类taskModule
 * The default module factory.
 * Note that in a graphical interface, Taskcell will derive a subclass
 * that supports graphical design taskmodule
 * \return 新建的模块运行时对象。The new module run-time object.
 */
taskCell * taskProject::default_NewCell()
{
	return new taskCell();
}
/*!
 * \brief taskProject::default_InsAppended 默认的后续创建初始化函数
 * Default subsequent creation of the initialization function
 * \param pmod 创建的模块 The module created
 * \param pnod 模块对应的运行时 The runtime of the corresponding module
 * \param pt 模块位置 Module graphics location
 */
void taskProject::default_InsAppended(taskCell * /*pmod*/, taskNode * /*pnod*/,QPointF /*pt*/)
{

}
void taskProject::default_Indexrefreshed()
{

}

QPointF	taskProject::default_GetCellPos(int n)
{
	int row = n / 4;
	return QPointF((n%4)*100+100,row * 200+100);
}

/*!
 * \brief taskProject::slot_outside_send 收到了从外部得到的数据包
 * Received a packet from the outside
 * \param node 是不是给本节点的 whether or not given to this node
 * \param arr 数据包 package
 */
void taskProject::slot_outside_recieved(QByteArray pkg)
{
	if (static_cast<size_t>(pkg.size())<sizeof(TASKBUS::subject_package_header))
		return;
	const TASKBUS::subject_package_header * header =
			reinterpret_cast<const TASKBUS::subject_package_header *>(pkg.constData());
	if (header->data_length+sizeof( TASKBUS::subject_package_header)!=static_cast<size_t>(pkg.size()))
		return;
	try {
		if (header->subject_id!=0xffffffff)
		{
			//进行解析、转发 To parse, forward
			if (m_iface_outside2inside_in.contains(header->subject_id)==false)
				throw tr("No valid inside maps for inid %1 .").arg(header->subject_id);
			unsigned int intenal_in_id = m_iface_outside2inside_in[header->subject_id];
			//专题是否被登记为内部专题 Whether the topic was registered as an internal topic
			if (m_hang_in2instance.contains(intenal_in_id)==true)
			{
				//是否为合法 Whether it is legal
				if (!(header->prefix[0]!=0x3C || header->prefix[1]!=0x5A || header->prefix[2]!=0x7E || header->prefix[3]!=0x69))
				{
					TASKBUS::subject_package_header * wrheader =
							reinterpret_cast<TASKBUS::subject_package_header *>(pkg.data());
					wrheader->subject_id = intenal_in_id;
					//广播 transmit
					int bidx = m_idx_instance2vec[m_hang_in2instance[intenal_in_id]];
					emit sig_cmd_write(m_vec_nodes[bidx],pkg);
				}
			}
			else
				throw tr("outside input subject \"%1\" does not exits.").arg(header->subject_id);

		}
		else
		{
			//信令
			const char * cmd = pkg.constData() + sizeof( TASKBUS::subject_package_header);
			QString dac = QString::fromUtf8(cmd,header->data_length);
			QMap<QString,QVariant> vt_map = taskCell::string_to_map(dac);
			slot_new_command(vt_map);
		}
	}
	catch(QString msg)
	{
		push_msg(msg);
	}
	catch(const char * msg)
	{
		push_msg(QString(msg));
	}
}

void taskProject::turnOnDebug(int nidx)
{
	if (nidx>=0 && nidx<m_vec_nodes.size())
		m_vec_nodes[nidx]->setDebug(true);
}

void taskProject::turnOffDebug(int nidx)
{
	if (nidx>=0 && nidx<m_vec_nodes.size())
		m_vec_nodes[nidx]->setDebug(false);
}

int  taskProject::get_nice(int nidx) const
{
	int nice = 2;
	if (nidx>=0 && nidx<m_vec_nodes.size())
	{
		QMap<QString,QVariant> addArgs = m_vec_cells[nidx]->additional_paras(m_vec_cells[nidx]->function_firstname());
		if (addArgs.contains("nice"))
			nice = addArgs["nice"].toInt();
	}
	return nice;
}
int  taskProject::set_nice(int  nidx,int nice)
{
	if (nidx>=0 && nidx<m_vec_nodes.size())
	{
		QMap<QString,QVariant> addArgs = m_vec_cells[nidx]->additional_paras(m_vec_cells[nidx]->function_firstname());
		addArgs["nice"] = nice;
		m_vec_cells[nidx]->set_additional_paras(m_vec_cells[nidx]->function_firstname(),addArgs);
	}
	return nice;
}

void taskProject::timerEvent(QTimerEvent * evt)
{
	if (evt->timerId()==m_nTimerID)
			flush_msg();
	QObject::timerEvent(evt);
}

void taskProject::push_msg(QString smsg)
{
	m_bufferMsgs.push_back(QByteArray::fromStdString(smsg.toStdString()));
	m_bufferMsgSources.push_back(QString("Project:"));
}
void taskProject::send_msg(QString smsgSource, QByteArrayList lst)
{
	static clock_t last_ck = clock();
	//Prevent of too short freq.
	clock_t curr_ck = clock();
	bool keep = false;
	if (curr_ck-last_ck <=CLOCKS_PER_SEC/10 && curr_ck-last_ck>=0)
		keep = true;
	last_ck = curr_ck;

	const int szMsgs = lst.size();
	for(int i=0;i<szMsgs;++i)
		m_bufferMsgSources.push_back(smsgSource);
	m_bufferMsgs.append(lst);

	if (keep==false)
		flush_msg();

}
void taskProject::flush_msg()
{
	if (m_bufferMsgs.size())
	{
		emit sig_message(m_bufferMsgSources,m_bufferMsgs);
		m_bufferMsgSources.clear();
		m_bufferMsgs.clear();
	}
}
