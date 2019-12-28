#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <string>
#include <vector>
#include <map>
#include <list>
#include <algorithm>
#include <iterator>
#include <set>
#include <unordered_map>
#include "cmdlineparser.h"
#include "tb_interface.h"
#include "fftw3.h"
using namespace TASKBUS;
const int OFFLINEDEBUG = 0;
//数据源方法
int do_fftw(const cmdlineParser & args);
//全局的终止标记
static bool bfinished = false;
using namespace std;
int main(int argc , char * argv[])
{
	//重要！设置输入输出为二进制！
	init_client();

	//解释命令行
	cmdlineParser args;
	if (OFFLINEDEBUG==0)
		args.parser(argc,argv);
	else
	{
		FILE * old_stdin, *old_stdout;
		auto ars = debug("D:/log/pid1290",&old_stdin,&old_stdout);
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
	else if (args.contains("function"/*,"tranform_fft"*/))//正常运行模式
	{
		ret = do_fftw(args);
	}
	else
	{
		fprintf(stderr,"Error:Function does not exits.");
		ret = -1;
	}

	return ret;
}


int do_fftw(const cmdlineParser & args)
{
	using namespace TASKBUS;
	int res = 0;
	//获得平台告诉自己的实例名
	unsigned int instance	  = args.toInt("instance",0);
	unsigned int isource	  = args.toInt("signal",0);
	unsigned int FFT		  = args.toInt("FFT",0);
	unsigned int Spec = args.toInt("Spec",0);
	unsigned int itmstamp_in  = args.toUInt("tmstamp_in",0);
	unsigned int itmstamp_out = args.toUInt("tmstamp_out",0);
	unsigned int itypes = args.toUInt("input_type",0);//0=real,1=complex
	//工作模式
	const int sptype	=	args.toInt("sptype",0);					fprintf(stderr,"sptype is %d.",sptype);

	int channels	=	args.toInt("channels",1);
	if (channels<1)
		channels = 1;

	//fftsize
	const int fftsize	=  args.toInt("fftsize",1024);				fprintf(stderr,"fftsize is %d.",fftsize);
	if (fftsize<=16)
	{
		fprintf(stderr,"fftsize must >16.");
		return res;
	}

	fflush(stderr);

	try{
		//判断参数合法性
		if (instance==0)	throw "function=quit;{\"error\":\"instance is 0, quit.\"}";


		//double * in;
		fftw_complex *out, *in;
		fftw_plan p;
		in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * fftsize);
		out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * fftsize);
		p = fftw_plan_dft_1d(fftsize, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

		vector<double> vec_fft_abs;
		unordered_map<unsigned int, unsigned long long> map_tmst_inside;
		unordered_map<unsigned int, unsigned long long> map_tmst_outside;

		for (int i=0;i<fftsize;++i)
			vec_fft_abs.push_back(0);

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
				map_tmst_outside[header.path_id] = *pdta;
			}
			else if (header.subject_id == isource)
			{
				if (itmstamp_out>0)
				{
					if (map_tmst_outside.find(header.path_id)==map_tmst_outside.end())
					{
						const unsigned long long llts = map_tmst_inside[header.path_id];
						push_subject(itmstamp_out,header.path_id,sizeof(unsigned long long),(const unsigned char *)&llts);
					}
					else
					{
						const unsigned long long llts = map_tmst_outside[header.path_id];
						push_subject(itmstamp_out,header.path_id,sizeof(unsigned long long),(const unsigned char *)&llts);
					}
				}

				const unsigned char * pdta = packagedta.data();
				//Normalizer
				double nmr = fftsize;
				if (itypes==0)
				{
					//数据类型转换
					switch (sptype)
					{
					case 0:
					{
						const int nPts = header.data_length / sizeof(short)/channels;
						const short * pdata = (const short *)pdta;
						for (int j=0;j<fftsize;++j)
						{
							in[j][0] = 0;
							for (int k = 0;k<channels;++k)
								in[j][0] += (j>=0 && j <nPts)?pdata[j*channels+k]:0;
							in[j][1] = 0;
							in[j][0] *= 0.54 - 0.46 * cos(2*3.1415927 * j / (min(nPts,fftsize)-1));
						}
						nmr *= 16384*16384;
					}
						break;
					case 1:
					{
						const int nPts = header.data_length / sizeof(short)/channels;
						for (int j=0;j<fftsize;++j)
						{
							double pd = 0;
							if (j>=0 && j <nPts)
							{
								for (int k = 0;k<channels;++k)
									pd += pdta[(j*channels+k)*2]*256+pdta[(j*channels+k)*2+1];
							}
							in[j][0] = pd;
							in[j][1] = 0;
							in[j][0] *= 0.54 - 0.46 * cos(2*3.1415927 * j / (min(nPts,fftsize)-1));
						}
						nmr *= 16384*16384;
					}
						break;
					case 2:
					{
						const int nPts = header.data_length / sizeof(char)/channels;
						const char * pdata = (const char *)pdta;
						for (int j=0;j<fftsize;++j)
						{
							in[j][0] = 0;
							for (int k = 0;k<channels;++k)
								in[j][0] += (j>=0 && j <nPts)?pdata[j*channels+k]:0;
							in[j][1] = 0;
							in[j][0] *= 0.54 - 0.46 * cos(2*3.1415927 * j / (min(nPts,fftsize)-1));
						}
						nmr *= 128*128;
					}
						break;
					case 3:
					{
						const int nPts = header.data_length / sizeof(char)/channels;
						const unsigned char * pdata = (const unsigned char *)pdta;
						for (int j=0;j<fftsize;++j)
						{
							in[j][0] = 0;
							for (int k = 0;k<channels;++k)
								in[j][0] += (j>=0 && j <nPts)?pdata[j*channels+k]:0;
							in[j][1] = 0;
							in[j][0] *= 0.54 - 0.46 * cos(2*3.1415927 * j / (min(nPts,fftsize)-1));
						}
						nmr *= 256*256;
					}
						break;
					default:
						break;
					}
				}
				else
				{
					//数据类型转换
					switch (sptype)
					{
					case 0:
					{
						const int nPts = header.data_length / sizeof(short)/channels/2;
						const short * pdata = (const short *)pdta;
						for (int j=0;j<fftsize;++j)
						{
							in[j][0] = 0;
							for (int k = 0;k<channels;++k)
							{
								in[j][0] += (j>=0 && j <nPts)?pdata[(j*channels+k)*2]:0;
								in[j][1] += (j>=0 && j <nPts)?pdata[(j*channels+k)*2+1]:0;
							}
							in[j][1] *= 0.54 - 0.46 * cos(2*3.1415927 * j / (min(nPts,fftsize)-1));
							in[j][0] *= 0.54 - 0.46 * cos(2*3.1415927 * j / (min(nPts,fftsize)-1));
						}
						nmr *= 16384*16384;;
					}
						break;
					case 1:
					{
						const int nPts = header.data_length / sizeof(short)/channels/2;
						for (int j=0;j<fftsize;++j)
						{
							double pd[2] = {0,0};
							if (j>=0 && j <nPts)
							{
								for (int k = 0;k<channels;++k)
								{
									pd[0] += pdta[(j*channels+k)*2*2]*256+pdta[(j*channels+k)*2*2+1];
									pd[1] += pdta[(j*channels+k)*2*2+2]*256+pdta[(j*channels+k)*2*2+3];
								}
							}
							in[j][0] = pd[0];
							in[j][1] = pd[1];
							in[j][0] *= 0.54 - 0.46 * cos(2*3.1415927 * j / (min(nPts,fftsize)-1));
							in[j][1] *= 0.54 - 0.46 * cos(2*3.1415927 * j / (min(nPts,fftsize)-1));
						}
						nmr *= 16384*16384;;
					}
						break;
					case 2:
					{
						const int nPts = header.data_length / sizeof(char)/channels/2;
						const char * pdata = (const char *)pdta;
						for (int j=0;j<fftsize;++j)
						{
							in[j][0] = 0;
							in[j][1] = 0;
							for (int k = 0;k<channels;++k)
							{
								in[j][0] += (j>=0 && j <nPts)?pdata[(j*channels+k)*2]:0;
								in[j][1] += (j>=0 && j <nPts)?pdata[(j*channels+k)*2+1]:0;
							}
							in[j][1] *= 0.54 - 0.46 * cos(2*3.1415927 * j / (min(nPts,fftsize)-1));
							in[j][0] *= 0.54 - 0.46 * cos(2*3.1415927 * j / (min(nPts,fftsize)-1));
						}
						nmr *= 128*128;;
					}
						break;
					case 3:
					{
						const int nPts = header.data_length / sizeof(char)/channels/2;
						const unsigned char * pdata = (const unsigned char *)pdta;
						for (int j=0;j<fftsize;++j)
						{
							in[j][0] = 0;
							in[j][1] = 0;
							for (int k = 0;k<channels;++k)
							{
								in[j][0] += (j>=0 && j <nPts)?pdata[(j*channels+k)*2]:0;
								in[j][1] += (j>=0 && j <nPts)?pdata[(j*channels+k)*2+1]:0;
							}
							in[j][1] *= 0.54 - 0.46 * cos(2*3.1415927 * j / (min(nPts,fftsize)-1));
							in[j][0] *= 0.54 - 0.46 * cos(2*3.1415927 * j / (min(nPts,fftsize)-1));
						}
						nmr *= 256*256;;
					}
						break;
					default:
						break;
					}
				}
				fftw_execute(p); /* repeat as needed */
				for (int j=0;j<fftsize;++j)
				{
					const double ab = sqrt(out[j][0] * out[j][0] + out[j][1] * out[j][1]);
					const double a =  10 * log(ab/nmr)/log(10.0);
					if (itypes==0)
						vec_fft_abs[j] = a;
					else
						vec_fft_abs[(j+fftsize/2)%fftsize] = a;
				}
				//output
				if (FFT>0)
					push_subject(FFT,header.path_id,fftsize*sizeof(double)/(itypes==0?2:1),(const unsigned char *)vec_fft_abs.data());
				if (Spec>0)
					push_subject(Spec,header.path_id,fftsize*sizeof(double),(const unsigned char *)out);
				++map_tmst_inside[header.path_id];
			}
		}

		fftw_destroy_plan(p);
		fftw_free(in); fftw_free(out);
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

