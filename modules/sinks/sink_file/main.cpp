#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <string>
#include <vector>
#include <map>
#include <list>
#include <algorithm>
#include <iterator>
#include <QLocale>
#include <set>
#include <QDir>
#include <QDateTime>
#include "cmdlineparser.h"
#include "tb_interface.h"


using namespace TASKBUS;
//#define OFFLINEDEBUG

unsigned long long pack_num = 0;

int do_sink_txt(const cmdlineParser & args);
int do_sink_bin(const cmdlineParser & args);
//全局的终止标记
bool bfinished = false;
using namespace std;
int main(int argc , char * argv[])
{
	QCoreApplication a(argc, argv);
	init_client();
#ifdef OFFLINEDEBUG
	FILE * old_stdin, *old_stdout;
	auto ars = debug("D:/log/pid1290",&old_stdin,&old_stdout);
	const  cmdlineParser args (ars);
#else
	const cmdlineParser args (argc,argv);
#endif
	int ret = 0;

	//每个模块要响应 --information参数,打印自己的功能定义字符串。或者提供一个json文件。
	if (args.contains("information"))
	{		
		QFile fp(":/json/sink_file."+QLocale::system().name()+".json");
		if (fp.open(QIODevice::ReadOnly)==false)
		{
			fp.setFileName(":/json/sink_file.exe.json");
			fp.open(QIODevice::ReadOnly);
		}
		if (fp.isOpen())
		{
			QByteArray arr = fp.readAll();
			arr.push_back('\0');
			puts(arr.constData());
			fflush(stdout);
		}
		ret = -1;
	}
	else if (args.contains("function","sink_filecsv"))//正常运行模式
		ret = do_sink_txt(args);
	else if (args.contains("function","sink_filebin"))//正常运行模式
		ret = do_sink_bin(args);
	else
	{
		fprintf(stderr,"Error:Function does not exits.");
		ret = -1;
	}

	return ret;
}

template <typename T>
void append_text(const vector<unsigned char> & pkd, QTextStream & st, const int typen)
{
	//"0:uint8 1:int8 2:uint16 3:int16 4:uint32 5:int32 6:uint64 7:int64 8:float 9:double 10:char"
	const size_t sz_elem = sizeof(T);
	const size_t sz_total = pkd.size()/sz_elem;
	if (sz_total==0)
	{
		st<<",0,";
		return;
	}
	const T * pDt = (const T*) pkd.data();
	st<<","<<sz_total<<",\"";
	for (size_t i = 0; i<sz_total;++i)
	{
		if (typen!=10 && i>0)
			st<<",";
		st<<pDt[i];
	}
	st<<"\"";

}


int do_sink_txt(const cmdlineParser & args)
{
	using namespace TASKBUS;
	int res = 0;
	//获得平台告诉自己的实例名

	const int instance	  = args.toInt("instance",0);
	const QString folder	=	QString::fromStdString(args.toString("folder","./sink_file/"));
	const int maxsize = args.toInt("maxsize",0);
	map<int,int> map_subs;
	map<int,QFile> map_files;
	QDir dir("/");
	//各路配置
	for (int i=0;i<256;++i)
	{
		char buf_key[256];
		sprintf(buf_key,"out%d",i);
		const int v = args.toInt(buf_key,0);
		sprintf(buf_key,"type%d",i);
		const int t = args.toInt(buf_key,0);
		if (v>0)
			map_subs[v] = t;
	}

	try{
		//判断参数合法性
		if (instance==0)	throw "function=quit;{\"error\":\"instance is 0, quit.\"}";
		int failed_header = 0;
		while (false==bfinished)
		{
			subject_package_header header;
			vector<unsigned char> packagedta = pull_subject(&header);
			if (packagedta.size()==0)
			{
				if (++failed_header>16)
					bfinished = true;
				continue;
			}
			if (is_valid_header(header)==false)
			{
				if (++failed_header>16)
					bfinished = true;
				continue;
			}
			if ( is_control_subject(header))
			{
				//收到命令进程退出的广播消息,退出
				if (strstr(control_subject(header,packagedta).c_str(),"function=quit;")>=0)
					bfinished = true;
				continue;
			}
			if (map_subs.find(header.subject_id)==map_subs.end())
				continue;
			if (map_files.find(header.subject_id)==map_files.end())
			{
				QFileInfo ifd(folder);
				dir.mkpath(ifd.absoluteFilePath());
				QString bufm = ifd.absoluteFilePath();
				bufm += "/";
				QString buf_tmp = QString("Ins%1_Sub%2_Time_%3.csv").arg(instance)
						.arg(header.subject_id)
						.arg(QDateTime::currentDateTime().toString("yyyyMMddHHmmss"));
				bufm += buf_tmp;

				map_files[header.subject_id].setFileName(bufm);
				if (map_files[header.subject_id].open(QIODevice::WriteOnly))
				{
					QTextStream st(&(map_files[header.subject_id]));
					st<<"package,Instance,Subject,Path,Size,Values\n";
					st.flush();
				}
				else
					continue;
			}
			QFile & fp = map_files[header.subject_id];
			if (fp.isOpen()==false)
			{
				map_files.erase(header.subject_id);
				continue;
			}
			QTextStream st(&fp);
			st<<","<<pack_num++;
			st<<","<<instance;
			st<<","<<","<<header.subject_id;
			st<<","<<header.path_id;
			////"0:uint8 1:int8 2:uint16 3:int16 4:uint32 5:int32 6:uint64 7:int64 8:float 9:double 10:char"
			switch(map_subs[header.subject_id])
			{
			case 0:
				append_text<unsigned char>(packagedta,st,map_subs[header.subject_id]);
				break;
			case 1:
				append_text<char>(packagedta,st,map_subs[header.subject_id]);
				break;
			case 2:
				append_text<unsigned short>(packagedta,st,map_subs[header.subject_id]);
				break;
			case 3:
				append_text<short>(packagedta,st,map_subs[header.subject_id]);
				break;
			case 4:
				append_text<unsigned int>(packagedta,st,map_subs[header.subject_id]);
				break;
			case 5:
				append_text<int>(packagedta,st,map_subs[header.subject_id]);
				break;
			case 6:
				append_text<unsigned long long>(packagedta,st,map_subs[header.subject_id]);
				break;
			case 7:
				append_text<long long>(packagedta,st,map_subs[header.subject_id]);
				break;
			case 8:
				append_text<float>(packagedta,st,map_subs[header.subject_id]);
				break;
			case 9:
				append_text<double>(packagedta,st,map_subs[header.subject_id]);
				break;
			case 10:
				append_text<char>(packagedta,st,map_subs[header.subject_id]);
				break;
			default:
				append_text<unsigned char>(packagedta,st,4);
				break;
			}
			st<<"\n";
			st.flush();

			if (fp.size()>=maxsize && maxsize>0)
			{
				fp.close();
				map_files.erase(header.subject_id);
			}
		}

	}
	catch (const char * errMessage)
	{
		//向所有部位广播，偶要退出。
		push_subject(control_subect_id(),/*instance,broadcast_destin_id(),*/0,errMessage);
		fprintf(stderr,"Error:%s.",errMessage);
		fflush (stderr);
		res = -1;
	}
	return res;
}



int do_sink_bin(const cmdlineParser & args)
{
	using namespace TASKBUS;
	int res = 0;
	//获得平台告诉自己的实例名
	const int maxsize = args.toInt("maxsize",0);
	const int instance	  = args.toInt("instance",0);
	const QString folder	=	QString::fromStdString(args.toString("folder","./sink_file/"));
	map<int,int> map_subs;
	map<int,map<int,QFile> > map_files;
	QDir dir("/");
	//各路配置
	for (int i=0;i<256;++i)
	{
		char buf_key[256];
		sprintf(buf_key,"out%d",i);
		const int v = args.toInt(buf_key,0);
		sprintf(buf_key,"type%d",i);
		const int t = args.toInt(buf_key,0);
		if (v>0)
			map_subs[v] = t;
	}

	try{
		//判断参数合法性
		if (instance==0)	throw "function=quit;{\"error\":\"instance is 0, quit.\"}";
		int failed_header = 0;
		while (false==bfinished)
		{
			subject_package_header header;
			vector<unsigned char> packagedta = pull_subject(&header);
			if (packagedta.size()==0)
			{
				if (++failed_header>16)
					bfinished = true;
				continue;
			}
			if (is_valid_header(header)==false)
			{
				if (++failed_header>16)
					bfinished = true;
				continue;
			}
			if ( is_control_subject(header))
			{
				//收到命令进程退出的广播消息,退出
				if (strstr(control_subject(header,packagedta).c_str(),"function=quit;")>=0)
					bfinished = true;
				continue;
			}
			if (map_subs.find(header.subject_id)==map_subs.end())
				continue;
			if (map_files.find(header.subject_id)==map_files.end())
			{
				QFileInfo ifd(folder);
				dir.mkpath(ifd.absoluteFilePath());
				QString bufm = ifd.absoluteFilePath();
				bufm += "/";
				QString buf_tmp = QString("Ins%1_Sub%2_P%3_%4.bin").arg(instance)
						.arg(header.subject_id).arg(header.path_id)
						.arg(QDateTime::currentDateTime().toString("yyyyMMddHHmmss"));
				bufm += buf_tmp;

				map_files[header.subject_id][header.path_id].setFileName(bufm);
				if (false ==map_files[header.subject_id][header.path_id].open(QIODevice::WriteOnly))
					continue;
			}
			else if (map_files[header.subject_id].find(header.path_id)==map_files[header.subject_id].end())
			{
				QFileInfo ifd(folder);
				dir.mkpath(ifd.absoluteFilePath());
				QString bufm = ifd.absoluteFilePath();
				bufm += "/";
				QString buf_tmp = QString("Ins%1_Sub%2_P%3_%4.bin").arg(instance)
						.arg(header.subject_id).arg(header.path_id)
						.arg(QDateTime::currentDateTime().toString("yyyyMMddHHmmss"));
				bufm += buf_tmp;

				map_files[header.subject_id][header.path_id].setFileName(bufm);
				if (false ==map_files[header.subject_id][header.path_id].open(QIODevice::WriteOnly))
					continue;
			}

			QFile & fp = map_files[header.subject_id][header.path_id];
			if (fp.isOpen()==false)
			{
				map_files[header.subject_id].erase(header.path_id);
				continue;
			}

			if (map_subs[header.subject_id]!=0)
				fp.write((const char *)&header,sizeof(header));

			fp.write((char *)packagedta.data(),packagedta.size());
			if (fp.size()>maxsize && maxsize>0)
			{
				fp.close();
				map_files[header.subject_id].erase(header.path_id);
			}
		}

	}
	catch (const char * errMessage)
	{
		//向所有部位广播，偶要退出。
		push_subject(control_subect_id(),/*instance,broadcast_destin_id(),*/0,errMessage);
		fprintf(stderr,"Error:%s.",errMessage);
		fflush (stderr);
		res = -1;
	}
	return res;
}

