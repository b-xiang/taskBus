#ifndef SPECTRALGRAPHCORE_H
#define SPECTRALGRAPHCORE_H
#include <vector>
#include <map>
#include <list>
#include <memory>
namespace SPECGRAM_CORE{
	/*!
	 * \brief The enum_DownSpMethod enum
	 * This enum controls the method for image downsample.When a 4096x65536
	 * image is down-sampling to current view (384x512), single tranform data
	 * line can be directly picked by doen-sample ratio, or more complex,
	 * Max or Avg functions will be  introduced for each raw-data group.
	 * 这个枚举控制了从原始变换域图到视图的映射关系。简单的映射直接靠缩放率计算最近
	 * 的一条变换线。更为复杂的，可以选取多个接近的变换线并计算最大值或者平均值。
	 */
	enum enum_DownSpMethod{
		DS_SIMPLE = 0,
		DS_MAX = 1,
		DS_AVG = 2
	};
	enum enum_RawType{
		RT_REAL = 0,
		RT_COMPLEX = 1
	};

	/*!
	 * \brief The spectralGraphCore class
	 * A common method to look into some waveform is 2-D spectrogram
	 * This class provides a common spectrogram core funtion without GUI.
	 * Modern GUI should connect its display and controller(key,mouse) messages
	 * to this class.
	 * 一种研究波形的常见方法是二维波谱图。这个类没有GUI，而是实现了一个GUI波谱图所需
	 * 的大部分功能接口、内部实现。开发者需要把特定GUI的显示、控制（键鼠）消息和本类连接
	 * 起来，以便完成最终的显示操控。
	 * @author goldenhawking@163.com
	 * @date 2019-09-20
	 */
	template <typename tp_raw = short, typename tp_trans = double, typename tp_rgb = unsigned int>
	class spectroGramCore
	{
	public:
		spectroGramCore();
		virtual ~spectroGramCore();
	public:
		virtual bool setDataPara(
				const int TransSize,	//transform size.变换域点数
				const int StepSize = 0//原始域步进， TransSize/16
				);
		virtual void setDownSampleMethod(enum_DownSpMethod enumDownMethod);
		virtual void setRawType(enum_RawType rt){m_enumRawType = rt;}
		//return a single line
		std::vector<tp_rgb> get_line(const double raw_coord, const double widthcoord = 0, const bool force = false);
		//shrink buffer to time range
		void shrink(const double mincoord, const double maxcoord);
	protected:
		/*!
		 * \brief fetch_raw fetch raw data
		 * \param centerPoint	center point coordinate
		 * \return rawpoints.
		 */
		virtual void reset(){}
		virtual std::vector<tp_raw> fetch_raw(long long centerPoint) = 0;
		virtual std::vector<tp_trans> transform(const std::vector<tp_raw> & raw) = 0;
		virtual std::vector<tp_rgb> trans_value_to_rgb(const std::vector<tp_trans> & v) const = 0;
	public:
		virtual double raw_pt_to_coord(const long long v) const = 0;
		virtual long long raw_coord_to_pt(const double v) const = 0;
		virtual double trans_pt_to_coord(const long long v) const = 0;
		virtual long long trans_coord_to_pt(const double v) const = 0;

	public:
		enum_RawType rawType() const {return m_enumRawType;}
		int transSize() const {return m_nTransSize;}
		int StepSize() const {return m_nStepSize;}
	protected://Data manage
		enum_DownSpMethod m_enumDownMethod = DS_AVG;
		enum_RawType m_enumRawType = RT_REAL;
		/*!
		 * \brief m_trans_buf
		 * main buf,
		 * timestamp index (Sample Points groups,etc)-->transform line groups.
		 * Transform line may be a fft window.  fetch_raw will be called when
		 * buf does not contains a special timepoint.
		 * 主缓存，
		 * 窗口中心时戳组ID（一般是样点组，参考m_nStepSize）-->变换域线。
		 * 变换域可以是一次FFT窗口对应的数据,是一个缓存。发现木有的时候会请求
		 */
		std::map<long long, std::vector<tp_trans> > m_trans_buf;
		//transform domain width (FFT Size) 变换域宽度,如FFT后的点数
		int m_nTransSize = 0;
		//Raw data Group Size 原始域的组大小。
		int m_nStepSize = 0;
	private:
		long long m_nBufMax = 1024*1024*16;
		long long m_last_center = 0;
	};
	template <typename tp_raw, typename tp_trans, typename tp_rgb>
	spectroGramCore<tp_raw,tp_trans,tp_rgb>::spectroGramCore()
	{

	}

	template <typename tp_raw, typename tp_trans, typename tp_rgb>
	spectroGramCore<tp_raw,tp_trans,tp_rgb>::~spectroGramCore()
	{

	}
	template <typename tp_raw, typename tp_trans, typename tp_rgb>
	void spectroGramCore<tp_raw,tp_trans,tp_rgb>::setDownSampleMethod(enum_DownSpMethod enumDownMethod)
	{
		if (m_enumDownMethod!=enumDownMethod)
		{
			m_enumDownMethod = enumDownMethod;
			m_trans_buf.clear();
		}

	}
	template <typename tp_raw, typename tp_trans, typename tp_rgb>
	bool spectroGramCore<tp_raw,tp_trans,tp_rgb>::setDataPara(
			const int TransSize,
			const int StepSize //= 0 default  as RawWinSize/16 .变换域点数,默认为 RawWinSize/16
			)
	{
		const int newGroupTest = StepSize==0?TransSize/4:StepSize;
		const int newGroupS = newGroupTest==0?1:newGroupTest;
		if (TransSize==m_nTransSize&& newGroupS==m_nStepSize)
			return true;
		if (TransSize==0)
			return false;
		if (newGroupS==0)
			return false;
		m_nTransSize = TransSize;
		m_nStepSize = newGroupS;
		m_trans_buf.clear();
		reset();
		return true;
	}

	template <typename tp_raw, typename tp_trans, typename tp_rgb>
	std::vector<tp_rgb> spectroGramCore<tp_raw,tp_trans,tp_rgb>
	::get_line(const double raw_coord
			   ,const double widthcoord
			   ,const bool force)
	{
		std::vector<tp_rgb> v_res;
		if (!m_nStepSize)
			return v_res;
		//span
		//coord start
		const double raw_start = raw_coord - widthcoord/2 ;
		const double raw_end = raw_coord + widthcoord/2 ;
		const double raw_span = raw_end - raw_start;
		const double raw_center = raw_coord ;
		//raw points
		const long long raw_pt_start = raw_coord_to_pt(raw_start);
		const long long raw_pt_end = raw_coord_to_pt(raw_end);
		const long long raw_pt_center = raw_coord_to_pt(raw_center);
		//raw groups
		const long long raw_group_start = static_cast<long long>(raw_pt_start *1.0 /  m_nStepSize + .5);
		const long long raw_group_end = static_cast<long long>(raw_pt_end *1.0 /  m_nStepSize + .5);
		const long long raw_group_center = static_cast<long long>(raw_pt_center *1.0 /  m_nStepSize + .5);
		m_last_center = raw_group_center;
		if (m_enumDownMethod==DS_SIMPLE){
			if (m_trans_buf.find(raw_group_center)!=m_trans_buf.end()&&!force)
			{
				std::vector<tp_trans> & vec_trans
						= m_trans_buf[raw_group_center];
				v_res = trans_value_to_rgb(vec_trans);
				vec_trans.shrink_to_fit();
			}
			else
			{
				std::vector<tp_raw> vec_raw = fetch_raw(raw_pt_center);
				if (vec_raw.size())
				{
					std::vector<tp_trans> & vec_trans
							= m_trans_buf[raw_group_center]
							= transform(vec_raw);
					v_res = trans_value_to_rgb(vec_trans);
					vec_trans.shrink_to_fit();
				}
			}

		}
		else if (m_enumDownMethod==DS_MAX){
			std::vector<tp_trans> target_trans;
			for (long long g = raw_group_start; g<= raw_group_end;++g)
			{
				if (m_trans_buf.find(g)!=m_trans_buf.end()&&!force)
				{
					if (!target_trans.size())
						target_trans = m_trans_buf[g];
					else {
						const size_t szTrans = target_trans.size();
						std::vector<tp_trans> & vec_currtrans = m_trans_buf[g];
						const size_t szCurr = vec_currtrans.size();
						size_t sz = std::min(szCurr,szTrans);
						for (size_t i = 0;i<sz;++i)
							if (target_trans[i]<vec_currtrans[i])
								target_trans[i] = vec_currtrans[i];
					}
				}
				else
				{
					const long long currgp_centerPoints =  static_cast<long long>(
								g * m_nStepSize + m_nStepSize /2.0+.5);
					std::vector<tp_raw> vec_raw = fetch_raw(currgp_centerPoints);
					if (vec_raw.size())
					{
						m_trans_buf[g]
								= transform(vec_raw);
						const size_t szTrans = target_trans.size();
						std::vector<tp_trans> & vec_currtrans = m_trans_buf[g];
						const size_t szCurr = vec_currtrans.size();
						size_t sz = std::min(szCurr,szTrans);
						for (size_t i = 0;i<sz;++i)
							if (target_trans[i]<vec_currtrans[i])
								target_trans[i] = vec_currtrans[i];
						vec_currtrans.shrink_to_fit();
					}

				}
			}//end for
			v_res=trans_value_to_rgb(target_trans);
		}
		else if (m_enumDownMethod==DS_AVG){
			std::vector<tp_trans> target_trans;
			double nn = 0;
			for (long long g = raw_group_start; g<= raw_group_end;++g)
			{
				if (m_trans_buf.find(g)!=m_trans_buf.end()&&!force)
				{
					++nn;
					if (!target_trans.size())
						target_trans = m_trans_buf[g];
					else {
						const size_t szTrans = target_trans.size();
						std::vector<tp_trans> & vec_currtrans = m_trans_buf[g];
						const size_t szCurr = vec_currtrans.size();
						size_t sz = std::min(szCurr,szTrans);
						for (size_t i = 0;i<sz;++i)
							target_trans[i] += vec_currtrans[i];
					}
				}
				else
				{
					const long long currgp_centerPoints =  static_cast<long long>(
								g * m_nStepSize + m_nStepSize /2.0+.5);
					std::vector<tp_raw> vec_raw = fetch_raw(currgp_centerPoints);
					if (vec_raw.size())
					{
						m_trans_buf[g]
								= transform(vec_raw);
						const size_t szTrans = target_trans.size();
						std::vector<tp_trans> & vec_currtrans = m_trans_buf[g];
						const size_t szCurr = vec_currtrans.size();
						size_t sz = std::min(szCurr,szTrans);
						for (size_t i = 0;i<sz;++i)
							target_trans[i] += vec_currtrans[i];
						vec_currtrans.shrink_to_fit();
						++nn;
					}

				}
			}//end for
			const size_t szTrans = target_trans.size();
			if (nn>0)
				for (size_t i = 0 ; i< szTrans; ++i)
					target_trans[i]/=nn*1.0;
			v_res = trans_value_to_rgb(target_trans);
		}
		if ((long long)m_nTransSize * m_trans_buf.size() * sizeof(tp_trans)>= m_nBufMax )
			shrink(raw_start - raw_span, raw_end + raw_span );
		return v_res;
	}

	template <typename tp_raw, typename tp_trans, typename tp_rgb>
	void spectroGramCore<tp_raw,tp_trans,tp_rgb>::shrink(const double mincoord, const double maxcoord)
	{
		//span
		//coord start
		const double raw_start = mincoord ;
		const double raw_end = maxcoord ;
		//raw points
		const long long raw_pt_start = raw_coord_to_pt(raw_start);
		const long long raw_pt_end = raw_coord_to_pt(raw_end);
		//raw groups
		const long long raw_group_start = static_cast<long long>(raw_pt_start *1.0 /  m_nStepSize + .5);
		const long long raw_group_end = static_cast<long long>(raw_pt_end *1.0 /  m_nStepSize + .5);
		std::list <long long> dellist;
		for(auto p = m_trans_buf.begin();p!=m_trans_buf.end();++p)
			if (p->first< raw_group_start || p->first>raw_group_end)
				dellist.push_back(p->first);
		for(auto p = dellist.begin();p!=dellist.end();++p)
			m_trans_buf.erase(*p);
	}
}

#endif // SPECTRALGRAPHCORE_H
