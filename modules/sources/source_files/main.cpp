#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QDateTime>
#include <QFileInfo>
#include "listen_thread.h"
#include <QTextStream>
#include <time.h>
#include <QList>
#include <algorithm>
#include <iterator>
#include <set>
#include "cmdlineparser.h"
#include "tb_interface.h"
using namespace TASKBUS;
//数据源方法
int do_source(const cmdlineParser & args);
//全局的终止标记
bool bfinished = false;
using namespace std;

//#define OFFLINEDEBUG

int main(int argc , char * argv[])
{
	QCoreApplication a(argc, argv);
	init_client();
#ifdef OFFLINEDEBUG
	FILE * old_stdin, *old_stdout;
	auto ars = debug("D:\\Dynamic\\BigCompetition\\bin\\R6\\debug\\pid344",&old_stdin,&old_stdout);
	const  cmdlineParser args (ars);
#else
	const cmdlineParser args (argc,argv);
#endif

	int ret = 0;
	QTextStream stmerr(stderr);
	//每个模块要响应 --information参数,打印自己的功能定义字符串。或者提供一个json文件。
	if (args.contains("information"))
	{
		QFile fp(":/json/source_files.json");
		if (fp.open(QIODevice::ReadOnly))
		{
			QByteArray arr = fp.readAll();
			arr.push_back('\0');
			puts(arr.constData());
			fflush(stdout);
		}
		ret = -1;
	}
	else if (args.contains("function"))//正常运行模式
	{
		//用于接收消息的线程
		listen_thread * th = new listen_thread(&a);
		QObject::connect(th,&listen_thread::quit_app,&a,&QCoreApplication::quit);
		th->start();
		ret = do_source(args);
	}
	else
	{
		stmerr<<"Error:Function does not exits.\n";
		ret = -1;
	}

	return ret;
}


//枚举文件夹中的所有文件的函数，按照创建时间排序
QList<QString> enum_files(QString folder, QString typestr)
{
	QList<QString> vec_str_files;
	map<QDateTime,QList<QString> > map_files;
	//寻找最新的文件夹
	QDir dir(folder);
	QStringList fts;
	fts<<typestr;
	QFileInfoList lst = dir.entryInfoList(fts);
	foreach (QFileInfo info, lst)
		map_files[info.lastModified()].push_back(info.absoluteFilePath());

	for(auto p = map_files.begin();p!=map_files.end();++p)
		copy(p->second.begin(),p->second.end(),back_inserter(vec_str_files));

	return vec_str_files;
}

/*!  这个函数实现了一个用于测试的信号源。
 *   仅用于演示，对输入输出数据的适应性不好。
*/
int do_source(const cmdlineParser & args)
{
	using namespace TASKBUS;
	QTextStream stmerr(stderr);
	int res = 0;
	//获得平台告诉自己的实例名
	int instance	  = args.toInt("instance",0);
	int isource		  = args.toInt("source",0);
	int timestamp	  = args.toInt("timestamp",0);
	int encode		  = args.toInt("encode",0);
	//监视的文件夹
	if (args["folder"].size()!=1)
	{
		stmerr<<"folder must be setted, and only one folder is supported.\n";
		return res;
	}

	const QString str_folder = encode==0?QString::fromLocal8Bit(args.toString("folder",".\\data").c_str())
									   :QString::fromUtf8(args.toString("folder",".\\data").c_str());
	QDir dir_f(str_folder);
	const QString str_type = QString::fromStdString(args.toString("type","*.pcm"));

	//工作模式,0顺序，1重复
	const int para_mod	=	args.toInt("mode",0);
	//自动删除
	const int auto_del	=	args.toInt("auto_del",0);
	//接续
	const int frame_contines	=  args.toInt("frame_contines",1);
	//跳跃
	const int read_jump	=  args.toInt("read_jump",1);

	long long initial_offset = args.toInt64("initial_offset",0);

	if (read_jump<=0)
	{
		stmerr<<"read_jump must >0.\n";
		return res;
	}
	//帧长（字节）
	const int frame_len	=  args.toInt("frame_len",1);
	if (frame_len<=0)
	{
		stmerr<<"frame_len must >0.\n";
		return res;
	}
	//帧速率(帧/秒)
	const double frame_rate =  args.toDouble("frame_rate",100);
	if (frame_rate<=0)
	{
		stmerr<<"frame_rate must >0.";
		return res;
	}
	//保留最新的文件
	const int keep_last =  args.toInt("keep_last",1);

	try{
		//判断参数合法性
		if (instance==0)	throw "\"quit\":{\"error\":\"instance is 0, quit.\"}";
		set<QString> history;
		vector<unsigned char> data;
		for (int i=0;i<frame_len;++i)
			data.push_back(0);
		int curr_size = 0;
		long long curr_pos = 0;
		long long next_pos = -1;

		//帧速率
		clock_t start, finish;
		double  duration;
		start = clock();
		double total_frames = 0;
		unsigned long long file_offset_global = 0;
		while (bfinished==false)
		{
			//枚举文件
			QList<QString> files = enum_files(str_folder,str_type);
			//开始处理
			for (auto p = files.begin();p!=files.end();++p)
			{
				//如果需要保留最后一个不处理，则继续
				if(keep_last)
				{
					auto q = p;
					++q;
					if (q==files.end())
						continue;
				}
				QString fname = *p;
				//如果模式为顺序处理，则判断是不是要处理
				if (para_mod==0 && history.find(fname)!=history.end())
					continue;
				if (para_mod==0)
					history.insert(fname);

				QString full_path = fname;
				stmerr<<"Dealing file "<< full_path<<"\n";
				stmerr.flush();
				//打开文件
				QFile fp(full_path);
				if (fp.open(QIODevice::ReadOnly))
				{
					//看看是不是需要清空
					if (frame_contines==0)
					{
						curr_size = 0;
						curr_pos = 0;
					}
					const long long size_f = fp.size()-initial_offset;
					if(size_f<0)
						continue;
					if (curr_pos>0)
						fp.seek(curr_pos+initial_offset);
					else
						fp.seek(0+initial_offset);


					const int batch_read = 65536;
					unsigned char buffer_red[batch_read];
					long long nred = fp.read((char *)buffer_red,batch_read);
					while (nred>0)
					{
						for (int i=0;i<nred;++i)
						{
							data[curr_size] = buffer_red[i];
							++curr_size;
							//emit
							if (curr_size>=frame_len)
							{
								const unsigned long long fileoffset_current = file_offset_global + curr_pos;
								if (timestamp)
								{
									push_subject(
									timestamp,				//专题
									instance,				//咱就一路数据，干干净净,用自己的进程ID确保唯一性。
									sizeof(unsigned long long),
									(unsigned char *)&fileoffset_current
									);
								}
								if (isource)
								{
									push_subject(
									isource,				//专题
									instance,				//咱就一路数据，干干净净,用自己的进程ID确保唯一性。
									frame_len,
									(unsigned char *)data.data()
									);
								}
								++total_frames;
									//时间控制
								finish = clock();
								duration = (double)(finish - start)*1000 / CLOCKS_PER_SEC;//毫秒
								while (duration/total_frames < 1000.0/frame_rate)
								{
									QThread::msleep(20);
									finish = clock();
									duration = (double)(finish - start)*1000 / CLOCKS_PER_SEC;//毫秒
								}
								if (duration>=5000)
								{
									start = clock();
									finish = clock();
									total_frames = 0;
								}

								curr_size = 0;
								//在缓存内继续步进行不行？
								int next_buffpos = i - frame_len + read_jump;
								if (next_buffpos >=0 && next_buffpos <nred)
								{
									i = next_buffpos;
									curr_pos += read_jump;
									next_pos = -1;
								}
								else//下一个位置不在缓存范围内
								{
									next_pos = curr_pos + read_jump;
									break;
								}
							}
						}
						if (next_pos>0)//下一位置超过缓存
						{
							//下一个需要读取的位置在文件内部
							if (next_pos<size_f)
							{
								fp.seek(next_pos+initial_offset);
								nred = fp.read((char *)buffer_red,batch_read);
								curr_pos = next_pos;
								next_pos = -1;
							}
							else//下一个位置超过了文件
							{
								curr_pos = 0;
								next_pos = next_pos - size_f;//下一个文件头多少个之后
								break;
							}
						}
						else
							nred = fp.read((char *)buffer_red,batch_read);
					}
					file_offset_global += size_f;
					fp.close();
					if (next_pos>=0)//需要预读
					{
						curr_pos = next_pos;
						next_pos = -1;
					}
					else//记录起点，逻辑起点其实是在上一个文件的某个位置。
					{
						curr_pos = -curr_size;
						//file_offset_global -= curr_size;
					}
					//要不要删除
					if(auto_del)
						dir_f.remove(full_path);
				}
				//第二个文件取消
				initial_offset = 0;

			}

		}

	}
	catch (const char * errMessage)
	{
		//向所有部位广播，偶要退出。
		push_subject(control_subect_id(),/*instance,broadcast_destin_id(),*/0,errMessage);
		stmerr<<"Error: "<<errMessage;
		fflush (stderr);
		res = -1;
	}


	return res;
}


