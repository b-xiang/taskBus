#pragma once
#include <cstdio>
#include <cstring>
#include <cassert>
#include <vector>
#include <list>
#include <string>
#include <map>
#include <iterator>
#include <algorithm>
#include <regex>
#include <thread>
#include <mutex>
#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#endif
//Comments:
//Next feature: Multithread-push protect.
namespace TASKBUS{
	//数据专题包头结构体，1字节序对齐
	//Datahead structure with pack = 1
#pragma pack(push,1)
	struct subject_package_header{
		//Always be 0x3C,0x5A,0x7E,0x69。
		unsigned char  prefix[4];
		unsigned int subject_id;
		unsigned int path_id;
		unsigned int data_length;
	};
#pragma pack(pop)

	/*!
	 * \brief init_client A application should call initclient first.
	 */
	inline void init_client();
	//是否为控制指令 Whether to be a control instruction or a normal subject
	inline bool is_control_subject(const subject_package_header & header);
	inline bool is_valid_header(const subject_package_header & header);
	//返回控制信令专题 0xffffffff
	inline unsigned int control_subect_id();
	//用于方便连线模式的函数, line connect mod(subject) functions
	inline void push_subject(
			const unsigned int subject_id,
			const unsigned int path_id,
			const unsigned int data_length,
			const unsigned char   *dataptr
			);
	inline void push_subject(
			const unsigned int subject_id,
			const unsigned int path_id,
			const char   *dataptr
			);
	inline void push_subject(
			const unsigned char   *allptr,
			const unsigned int totalLength
			);
	inline void push_subject(
			const subject_package_header header,
			const unsigned char   *dataptr
			);
	//接收专题数据,push subject to stdout
	inline std::vector<unsigned char> pull_subject(
			subject_package_header * header
			);

	//调试，利用记录的打桩进程的stdin和命令行调试。会返回命令行
	inline std::vector<std::string> debug(const char * logpath, FILE ** old_stdin, FILE ** old_stdout);

	//用于方便操作指令的函数, text command mod functions
	//--------------------------------------------------------------------
#if  _MSC_VER >=1200 || __cplusplus >= 201103L
	inline std::string trim_elems(const std::string &  text);
	inline std::map<std::string, std::string> string_to_map(const std::string & s);
	inline std::map<std::string, std::string> ctrlpackage_to_map(const std::vector<unsigned char> & s);
	inline std::string map_to_string(const std::map<std::string, std::string> & s);
	inline std::vector<unsigned char> map_to_ctrlpackage(const std::map<std::string, std::string> & s);
#endif
	/**
	  *以下部分为具体实现。The following sections are the implementation
	  * -----------------------------------------------------------------------
	  * */
	/*!
	 * \brief init_client A application should call initclient first.
	 */
	inline void init_client()
	{
#ifdef WIN32
#if  _MSC_VER >=1200
		_setmode(_fileno(stdout),O_BINARY);
		_setmode(_fileno(stdin),O_BINARY);
#else
		setmode(fileno(stdout), O_BINARY);
		setmode(fileno(stdin), O_BINARY);
#endif
#endif
	}

	//推送专题数据
	inline void push_subject(
			const unsigned int subject_id,
			const unsigned int path_id,
			const unsigned int data_length,
			const unsigned char   *dataptr
			)
	{
		const unsigned char  prefix[4] = { 0x3C,0x5A,0x7E,0x69};
		fwrite(prefix,sizeof(char),4,stdout);
		fwrite(&subject_id,sizeof(subject_id),1,stdout);
		fwrite(&path_id,sizeof(path_id),1,stdout);
		fwrite(&data_length,sizeof(data_length),1,stdout);
		fwrite(dataptr,sizeof(unsigned char),data_length,stdout);
		fflush (stdout);
	}
	//推送专题数据
	inline void push_subject(
			const unsigned int subject_id,
			//const unsigned int source_id,
			//const unsigned int destin_id,
			const unsigned int path_id,
			const char   *dataptr
			)
	{
		const unsigned char  prefix[4] = { 0x3C,0x5A,0x7E,0x69};
		fwrite(prefix,sizeof(char),4,stdout);
		fwrite(&subject_id,sizeof(subject_id),1,stdout);
		fwrite(&path_id,sizeof(path_id),1,stdout);
		const unsigned int lenstr = strlen(dataptr)+1;
		fwrite(&lenstr,sizeof(lenstr),1,stdout);
		fwrite(dataptr,sizeof(unsigned char),lenstr,stdout);
		fflush (stdout);
	}
	//推送专题数据
	inline void push_subject(
			const subject_package_header header,
			const unsigned char   *dataptr
			)
	{
		assert(header.prefix[0]==0x3C&&header.prefix[1]==0x5A&&header.prefix[2]==0x7E&&header.prefix[3]==0x69);
		fwrite(&header,sizeof(header),1,stdout);
		fwrite(dataptr,sizeof(unsigned char),header.data_length,stdout);
		fflush (stdout);


	}
	inline void push_subject(
			const unsigned char   *allptr,
			const unsigned int totalLength
			)
	{
		assert(allptr[0]==0x3C&&allptr[1]==0x5A&&allptr[2]==0x7E&&allptr[3]==0x69);
		fwrite(allptr,sizeof(unsigned char),totalLength,stdout);
		fflush (stdout);

	}
	//接收专题数据
	inline std::vector<unsigned char> pull_subject(
			subject_package_header * header
			)
	{
		//读取缓存
		static const size_t batchdeal = 4096;
		std::vector<unsigned char> buf_data;
		//包头
		memset(header,0,sizeof(subject_package_header));
		fread(header,sizeof(subject_package_header),1,stdin);

		if(header->prefix[0]==0x3C&&header->prefix[1]==0x5A&&header->prefix[2]==0x7E&&header->prefix[3]==0x69)
		{
			//数据
			const size_t groups = header->data_length / batchdeal;
			unsigned char * buf =(unsigned char *) malloc(batchdeal);
			for (size_t i=0;i<groups;++i)
			{
				fread(buf,1,batchdeal,stdin);
				std::copy(buf,buf+batchdeal, std::back_inserter( buf_data ));
			}
			if (header->data_length % batchdeal)
			{
				fread(buf,1,header->data_length % batchdeal,stdin);
				std::copy(buf,buf+header->data_length % batchdeal, std::back_inserter( buf_data ));
			}
			free (buf);
			buf = 0;
		}
		return std::move(buf_data);
	}
	//用于方便操作指令的函数
	//是否为控制指令
	inline bool is_control_subject(const subject_package_header & header)
	{
		if(header.prefix[0]==0x3C&&header.prefix[1]==0x5A&&header.prefix[2]==0x7E&&header.prefix[3]==0x69)
			return (header.subject_id == 0xffffffff)?true:false;
		return false;
	}
	//提取控制命令
	inline std::string control_subject(const subject_package_header & header, const std::vector<unsigned char> & package)
	{
		std::string str;
		if(header.prefix[0]==0x3C&&header.prefix[1]==0x5A&&header.prefix[2]==0x7E&&header.prefix[3]==0x69)
		{
			if (header.data_length==package.size())
			{
				const size_t sz = package.size();
				for (size_t i=0;i<sz;++i)
					str.push_back((char)package[i]);
			}
		}
		return str;
	}

	inline bool is_valid_header(const subject_package_header & header)
	{
		if(header.prefix[0]==0x3C&&header.prefix[1]==0x5A&&header.prefix[2]==0x7E&&header.prefix[3]==0x69)
			return true;
		return false;
	}

	//返回控制信令专题
	inline unsigned int control_subect_id()
	{
		return 0xffffffff;
	}
	/** Debug the modile.
	*/
	inline std::vector<std::string> debug(const char * logpath, FILE ** old_stdin, FILE ** old_stdout)
	{
		std::vector<std::string> cmdline;
		std::string strpath = std::string(logpath);
		std::string fm_stderr = strpath + "/stderr.txt";
		std::string fm_stdin = strpath + "/stdin.dat";
		std::string fm_stdout = strpath + "/stdout.dat.debug";
#if  _MSC_VER >=1200
		if (old_stdin)
			freopen_s(old_stdin, fm_stdin.c_str(), "rb", stdin);
		else
		{
			FILE * olds = nullptr;
			freopen_s(&olds,fm_stdin.c_str(), "rb", stdin);
		}

		if (old_stdout)
			freopen_s(old_stdout, fm_stdout.c_str(), "wb", stdout);
		else
		{
			FILE * olds = nullptr;
			freopen_s(&olds,fm_stdout.c_str(), "wb", stdout);
		}

		//解析命令行
		FILE * fpErr = nullptr;
		fopen_s(&fpErr,fm_stderr.c_str(), "r");
#else
		if (old_stdin)
			*old_stdin = freopen(fm_stdin.c_str(),"rb",stdin);
		else
			freopen(fm_stdin.c_str(),"rb",stdin);

		if (old_stdout)
			*old_stdout = freopen(fm_stdout.c_str(),"wb",stdout);
		else
			freopen(fm_stdout.c_str(),"wb",stdout);

		//解析命令行
		FILE * fpErr = fopen(fm_stderr.c_str(),"r");

#endif
		if (fpErr)
		{
			char buf[65536];
			if (fgets(buf,65536,fpErr))
			{
				const int argc = atoi(buf);
				for (int i=0;i<argc;++i)
				{
					if (fgets(buf,65536,fpErr))
					{
						while(strlen(buf)>0)
						{
							if ( buf[strlen(buf)-1]<=32&&buf[strlen(buf)-1]>0)
								buf[strlen(buf)-1] = 0;
							else
								break;
						}
						if (strlen(buf)<=0)
							continue;
						char * swim = buf;
						while (*swim<=32 && *swim>0)
							++swim;
						if (strlen(swim)<=0)
							continue;
						cmdline.push_back(std::string(swim));
					}
					else
						break;
				}

			}
			fclose(fpErr);
		}
		return cmdline;

	}

#if  _MSC_VER >=1200 || __cplusplus >= 201103L
	/*!
	 * \brief trim_elems trim all spaces of  a string.
	 * \param text string to be trimmed.
	 * \return trimmed result
	 */
	inline std::string trim_elems(const std::string &  text)
	{
		std::string result(text);
		if(!result.empty())
		{
			result.erase(0, result.find_first_not_of(" \n\r\t"));
			result.erase(result.find_last_not_of(" \n\r\t") + 1);
		}
		return result;
	}

	/*!
	 * \brief string_to_map convert string cmd to map
	 * \param s string cmd, key=value paires splitted by ';'
	 * \return map
	 */
	inline std::map<std::string, std::string> string_to_map(const std::string & s)
	{
		using namespace std;
		std::map<std::string, std::string> res;
		regex regex_set{"([;])"};
		regex regex_sep{"([=])"};
		sregex_token_iterator it{s.begin(), s.end(), regex_set, -1};
		list<string> words{it, {}};

		for_each(words.begin(),words.end(),[&regex_sep,&res](string v){
			sregex_token_iterator itp {v.begin(), v.end(), regex_sep, -1};
			list<string> paras{itp, {}};
			string key , value;
			if (paras.size())
				key = trim_elems(*paras.begin());
			else
				return;
			paras.pop_front();
			if (paras.size())
				value = trim_elems(*paras.begin());
			res[key] = value;
		});
		return res;
	}

	inline std::map<std::string, std::string> ctrlpackage_to_map(const std::vector<unsigned char> & s)
	{
		using namespace std;
		std::string str;
		copy(s.begin(),s.end(),back_inserter(str));
		return string_to_map(str);
	}


	/*!
	 * \brief map_to_string convert map to key=value paires
	 * \param s map
	 * \return string cmd, key=value paires splitted by ';'
	 */
	inline std::string map_to_string(const std::map<std::string, std::string> & s)
	{
		using namespace std;
		string strv;
		for (auto p = s.begin(); p != s.end(); ++p)
		{
			strv+=(*p).first;
			strv+="=";
			strv+=(*p).second;
			strv+=";";
		}
		return strv;
	}

	inline std::vector<unsigned char> map_to_ctrlpackage(const std::map<std::string, std::string> & s)
	{
		std::string str = map_to_string(s);
		std::vector<unsigned char> v;
		copy(str.begin(),str.end(),std::back_inserter(v));
		v.push_back(0);
		return v;
	}

#endif
}

