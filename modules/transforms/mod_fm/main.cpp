#include <QCoreApplication>
#include <QVector>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <string>
#include "cmdlineparser.h"
#include "tb_interface.h"
using namespace TASKBUS;
const int OFFLINEDEBUG = 0;
//数据源方法
int do_mod_fm(const cmdlineParser & args);
//全局的终止标记
static bool bfinished = false;
using namespace std;
int main(int argc , char * argv[])
{
	QCoreApplication a(argc, argv);
	//重要！设置输入输出为二进制！
	init_client();

	//解释命令行
	cmdlineParser args;
	if (OFFLINEDEBUG==0)
		args.parser(argc,argv);
	else
	{
		FILE * old_stdin, *old_stdout;
		auto ars = debug("/home/goldenhawking/projects/taskbus/bin_linux_64/bin/debug/pid4113",&old_stdin,&old_stdout);
		args.parser(ars);
	}

	int ret = 0;
	//每个模块要响应 --information参数,打印自己的功能定义字符串。或者提供一个json文件。
	if (args.contains("information"))
	{
		extern const char * g_info ;
		puts(g_info);
		fflush(stdout);
		ret = -1;
	}
	else if (args.contains("function"/*,"mod_fm"*/))//正常运行模式
	{
		ret = do_mod_fm(args);
	}
	else
	{
		fprintf(stderr,"Error:Function does not exits.");
		ret = -1;
	}

	return ret;
}

#define Pi 3.14159265354
int do_mod_fm(const cmdlineParser & args)
{
	using namespace TASKBUS;
	int res = 0;
	//获得平台告诉自己的实例名
	const unsigned int instance	  = args.toInt("instance",0);
	const unsigned int isound	  = args.toInt("sound",0);
	const double in_spr			  = args.toDouble("in_spr",8000);
	const double out_spr		  = args.toDouble("out_spr",2500000);
	const unsigned int itmstamp_in  = args.toUInt("tmstamp_in",0);
	const unsigned int itmstamp_out = args.toUInt("tmstamp_out",0);
	const unsigned int osig  = args.toInt("signal",0);
	//工作模式
	const int sptype	=	args.toInt("sptype",0);					fprintf(stderr,"sptype is %d.",sptype);

	int channels	=	args.toInt("channels",1);
	if (channels<1)
		channels = 1;

	fflush(stderr);

	quint64 tmstmp = 0;

	try{
		//判断参数合法性
		if (instance==0)
			throw "function=quit;{\"error\":\"instance is 0, quit.\"}";
		int failed_header = 0;
		while (false==bfinished)
		{
			subject_package_header header;
			vector<unsigned char> packagedta = pull_subject(&header);
			if (is_valid_header(header)==false)
			{
				if (++failed_header>16)
					bfinished = true;
				continue;
			}
			if ( is_control_subject(header))
			{
				//收到命令进程退出的广播消息,退出
				if (strstr(control_subject(header,packagedta).c_str(),"function=quit;")!=nullptr)
					bfinished = true;
			}
			else if (header.subject_id==itmstamp_in)
			{
				const unsigned long long * pdta = (unsigned long long *) packagedta.data();
				//map_tmst_outside[header.path_id] = *pdta;
			}
			else if (header.subject_id == isound)
			{
				QVector<qint32> vec_buffer;
				const unsigned char * pdta = packagedta.data();
				//数据类型转换
				switch (sptype)
				{
				case 0:
				{
					const int nPts = header.data_length / sizeof(short)/channels;
					const short * pdata = (const short *)pdta;
					for (int j=0;j<nPts;++j)
					{
						int cp = 0;
						for (int k=0;k<channels;++k)
							cp += pdata[j*channels+k];
						cp /= channels;
						vec_buffer<<(cp);
					}
				}
					break;
				case 1:
				{
					const int nPts = header.data_length / sizeof(short)/channels;
					for (int j=0;j<nPts;++j)
					{
						int cb = 0;
						qint16 cp = 0;
						for (int k=0;k<channels;++k)
						{
							cp |= (pdta[(j*channels+k)*2]);
							cp |= (qint16(pdta[(j*channels+k)*2+1])<<8);
							cb += cp;
						}
						cb /= channels;
						vec_buffer<<(cb);
					}
				}
					break;
				case 2:
				{
					const int nPts = header.data_length / sizeof(char)/channels;
					const char * pdata = (const char *)pdta;
					for (int j=0;j<nPts;++j)
					{
						int cp = 0;
						for (int k=0;k<channels;++k)
							cp += pdata[j*channels+k];
						cp /= channels;
						vec_buffer<<(cp);
					}

				}
					break;
				case 3:
				{
					const int nPts = header.data_length / sizeof(char)/channels;
					const unsigned char * pdata = (const unsigned char *)pdta;
					for (int j=0;j<nPts;++j)
					{
						int cp = 0;
						for (int k=0;k<channels;++k)
							cp += int(pdata[j*channels+k])-128;
						cp /= channels;
						vec_buffer<<(cp);
					}

				}
					break;
				default:
					break;
				}
				//Time Stamp
				const double time_estab = vec_buffer.size()/in_spr;
				QVector<qint16> vec_output;
				const int totalSamples = vec_buffer.size();
				const double tm_stepOut = 1.0 / out_spr;
				const double K = 25.0/32767;
				for (double t = 0; t<= time_estab; t+=tm_stepOut,tmstmp+=tm_stepOut)
				{
					const double x_v = (t/time_estab) * totalSamples;
					const int x_left = x_v;
					const int x_right = (x_left<totalSamples-1)?(x_left+1):x_left;
					if (x_left<x_right)
					{
						const double v1 = vec_buffer[x_left];
						const double v2 = vec_buffer[x_right];
						const double v = v1 + (v2-v1) * (x_v-x_left)/(x_right-x_left);
						vec_output.push_back(cos(2*Pi*v*K)*1024);
						vec_output.push_back(-sin(2*Pi*v*K)*1024);
					}
					else
					{
						const double v = vec_buffer[x_left];
						vec_output.push_back(cos(2*Pi*v*K)*1024);
						vec_output.push_back(-sin(2*Pi*v*K)*1024);
					}

				}
				//播发
				if (osig)
				{
					push_subject(osig,instance,vec_output.size()*2,
								 (unsigned char *)vec_output.constData()
								 );
				}


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

