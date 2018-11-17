/************************************************************************/
/* 命令行参数解释器 by goldenhawking，可以用更先进的开源工具或者库代替。       */
/************************************************************************/
#pragma once
#include <string>
#include <unordered_map>
#include <list>
#include <set>
#include <vector>
#include <algorithm>
#include <cstring>
namespace TASKBUS{
	class cmdlineParser
	{
		typedef std::unordered_map<std::string,std::list<std::string> > t_dict;
		typedef std::unordered_map<std::string,std::set<std::string> > t_dict_index;
	public:

		cmdlineParser(void)
		{
		}

		~cmdlineParser(void)
		{
		}

		cmdlineParser(const int argc, const char * const argv[])
		{
			parser(argc,argv);
		}
		cmdlineParser(const std::vector<std::string> argv)
		{
			parser(argv);
		}

	public:
		//解译命令行为字典
		void parser(const int argc, const char * const argv[])
		{
			clear();
			std::string curr_key = argv[0];
			m_dict[curr_key];
			for (int i=1;i<argc;++i)
			{
				std::string currarg(argv[i]);
				const size_t sz_arglen = currarg.size();
				if (sz_arglen>2)
				{
					//分离参数名、参数值
					if (currarg[0]=='-')
					{
						size_t para_name_start = 0;
						size_t para_name_end = 0;
						while (para_name_start<sz_arglen && currarg[para_name_start]=='-' )
							++para_name_start,++para_name_start;
						while (para_name_end<sz_arglen && currarg[para_name_end]!='=')
							++para_name_end;
						std::string str_paraname = currarg.substr(para_name_start,para_name_end - para_name_start);
						if (para_name_end<sz_arglen)
						{
							const std::string str_val = currarg.substr(para_name_end+1,sz_arglen-para_name_end);
							m_dict[str_paraname].push_back(str_val);
							m_index[str_paraname].insert(str_val);
						}
						else
							m_dict[str_paraname];
						curr_key = str_paraname;
					}
					else
					{
						m_dict[curr_key].push_back(currarg);
						curr_key = argv[0];
					}
				}
			}
		}
		void parser(const std::vector<std::string> & argv)
		{
			clear();
			const int argc = argv.size();
			if (argc<1)
			    return;
			std::string curr_key = argv[0];
			m_dict[curr_key];
			for (int i=1;i<argc;++i)
			{
				std::string currarg(argv[i]);
				const size_t sz_arglen = currarg.size();
				if (sz_arglen>2)
				{
					//分离参数名、参数值
					if (currarg[0]=='-')
					{
						size_t para_name_start = 0;
						size_t para_name_end = 0;
						while (para_name_start<sz_arglen && currarg[para_name_start]=='-' )
							++para_name_start;
						while (para_name_end<sz_arglen && currarg[para_name_end]!='=')
							++para_name_end;
						std::string str_paraname = currarg.substr(para_name_start,para_name_end - para_name_start);
						if (para_name_end<sz_arglen)
						{
							const std::string str_val = currarg.substr(para_name_end+1,sz_arglen-para_name_end);
							m_dict[str_paraname].push_back(str_val);
							m_index[str_paraname].insert(str_val);
						}
						else
							m_dict[str_paraname];
						curr_key = str_paraname;
					}
					else
					{
						m_dict[curr_key].push_back(currarg);
						curr_key = argv[0];
					}
				}
			}
		}
		//返回所有获取的参数名
		std::set<std::string> keys() const
		{
			std::set<std::string> keys;
			for (auto p = m_dict.cbegin();p!=m_dict.cend();++p)
				keys.insert(p->first);
			return keys;
		}

		//返回是否含有一个参数
		bool contains(const std::string & spname) const
		{
			return m_dict.find(spname)==m_dict.end()?false:true;
		}

		//返回是否含有一个值
		bool contains(const std::string & spname, const std::string v) const
		{
			t_dict_index::const_iterator iter = m_index.find(spname);
			if (iter==m_index.end())
				return false;
			return iter->second.find(v)==iter->second.end()?false:true;
		}

		//支持使用 a["para"]的形式访问值的原始列表
		const std::list<std::string> & operator [] (const std::string key) const
		{
			static const std::list<std::string> empty;
			t_dict::const_iterator iter = m_dict.find(key);
			if (iter==m_dict.end())
				return empty;
			return iter->second;
		}
		//清理
		void clear()
		{
			m_dict.clear();
			m_index.clear();
		}

		//用于直接取得特定参数值。
		/*!
		spname 是参数名
		default_value 是默认值，即参数不存在时的默认值。
		argidx 是指定第几个参数值。有的  --选项可以带多个值。默认0表示取第一个
		*/
		int toInt(const std::string & spname, const int default_value, size_t argidx = 0) const
		{
			if (false==contains(spname))
				return default_value;
			if ((*this)[spname].size()<=argidx)
				return default_value;

			std::list<std::string>::const_iterator p = (*this)[spname].begin();
			for (size_t i=0;i<argidx;++i,++p);
			const std::string value = *p;
			int r = 0;
			if (strchr(value.c_str(),'e')!=0 ||strchr(value.c_str(),'E')!=0)
			{
				double k = atof(value.c_str());
				r = k;
			}
			else
				r = atoi(value.c_str());
			return r;
		}
		//用于直接取得特定参数值。
		/*!
		spname 是参数名
		default_value 是默认值，即参数不存在时的默认值。
		argidx 是指定第几个参数值。有的  --选项可以带多个值。默认0表示取第一个
		*/
		long long toInt64(const std::string & spname, const long long default_value, size_t argidx = 0) const
		{
			if (false==contains(spname))
				return default_value;
			if ((*this)[spname].size()<=argidx)
				return default_value;

			std::list<std::string>::const_iterator p = (*this)[spname].begin();
			for (size_t i=0;i<argidx;++i,++p);
			const std::string value = *p;
			long long r = 0;
			if (strchr(value.c_str(),'e')!=0 ||strchr(value.c_str(),'E')!=0)
			{
				double k = atof(value.c_str());
				r = k;
			}
			else
			{
#ifdef WIN32
				r = _atoi64(value.c_str());
#else
			    r = atoll(value.c_str());
#endif
			}
			return r;
		}		//用于直接取得特定参数值。
		/*!
		spname 是参数名
		default_value 是默认值，即参数不存在时的默认值。
		argidx 是指定第几个参数值。有的  --选项可以带多个值。默认0表示取第一个
		*/
		double toDouble(const std::string & spname, const double default_value, size_t argidx = 0) const
		{
			if (false==contains(spname))
				return default_value;
			if ((*this)[spname].size()<=argidx)
				return default_value;

			std::list<std::string>::const_iterator p = (*this)[spname].begin();
			for (size_t i=0;i<argidx;++i,++p);
			const std::string value = *p;
			return atof(value.c_str());
		}
		//用于直接取得特定参数值。
		/*!
		spname 是参数名
		default_value 是默认值，即参数不存在时的默认值。
		argidx 是指定第几个参数值。有的  --选项可以带多个值。默认0表示取第一个
		*/
		std::string toString(const std::string & spname, const std::string & default_value, size_t argidx = 0) const
		{
			if (false==contains(spname))
				return default_value;
			if ((*this)[spname].size()<=argidx)
				return default_value;

			std::list<std::string>::const_iterator p = (*this)[spname].begin();
			for (size_t i=0;i<argidx;++i,++p);
			const std::string value = *p;
			return value;
		}

	protected:
		t_dict m_dict;
		t_dict_index m_index;
	};
}
