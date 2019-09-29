#include "spectrogramfft.h"
#include <algorithm>
#include <cmath>
namespace SPECGRAM_CORE {
	spectroGramFFT::spectroGramFFT()
	{
	}
	spectroGramFFT::~spectroGramFFT()
	{
		if (m_plan)
			fftw_destroy_plan(m_plan);
		m_plan = nullptr;
		if (m_in)
			fftw_free(m_in);
		m_in = nullptr;
		if (m_out)
			fftw_free(m_out);
		m_out = nullptr;
	}
	std::vector<short> spectroGramFFT::fetch_raw(long long centerPoint)
	{
		std::vector<short> ret;
		if (centerPoint - m_nTransSize/2 - 1 < bufStart() ||
				centerPoint + m_nTransSize/2+1 > bufEnd())
			return ret;
		const int beginPt = centerPoint -  m_nTransSize/2 - bufStart();
		std::copy(m_vecRawBuf.begin()+beginPt,
				  m_vecRawBuf.begin()+beginPt+m_nTransSize,
				  std::back_inserter(ret));
		return ret;
	}
	void spectroGramFFT::reset()
	{
		if (m_plan)
			fftw_destroy_plan(m_plan);
		m_plan = nullptr;
		if (m_in)
			fftw_free(m_in);
		m_in = nullptr;
		if (m_out)
			fftw_free(m_out);
		m_out = nullptr;
		if (m_nTransSize==0)
			return;
		m_in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * m_nTransSize);
		m_out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * m_nTransSize);
		m_plan = fftw_plan_dft_1d(m_nTransSize, m_in, m_out, FFTW_FORWARD, FFTW_MEASURE);
	}
	std::vector<double> spectroGramFFT::transform(const std::vector<short> & raw)
	{
		std::vector<double> result;
		if (!m_nTransSize)
			return result;
		if (!m_plan)
		{
			reset();
			if (!m_plan)
				return result;
		}
		const int points = raw.size();
		int hammsize = (std::min(points,m_nTransSize)-1);
		if (hammsize<1)
			hammsize = 1;
		if (rawType()==RT_REAL)
		{
			for(int i=0;i<m_nTransSize;++i)
			{
				m_in[i][0] = m_in[i][1] = 0;
				if (i<points)
					m_in[i][0] = raw[i];
				m_in[i][0] *= 0.54 - 0.46 * cos(2*3.1415927 * i / hammsize);
			}
		}
		else if (rawType()==RT_COMPLEX)
		{
			for(int i=0;i<m_nTransSize;++i)
			{
				m_in[i][0] = m_in[i][1] = 0;
				if (i<points/2)
				{
					m_in[i][0] = raw[i*2];
					m_in[i][1] = raw[i*2+1];
				}
				m_in[i][0] *= 0.54 - 0.46 * cos(2*3.1415927 * i / hammsize);
				m_in[i][1] *= 0.54 - 0.46 * cos(2*3.1415927 * i / hammsize);
			}
		}
		fftw_execute(m_plan); /* repeat as needed */
		for (int j=0;j<m_nTransSize;++j)
		{
			const double a =  10 * log(sqrt(m_out[j][0] * m_out[j][0] + m_out[j][1] * m_out[j][1]))/log(10.0);
			result.push_back(a);
		}
		result.shrink_to_fit();
		return result;
	}
	std::vector<unsigned int> spectroGramFFT::trans_value_to_rgb(const std::vector<double> & v) const
	{
		std::vector<unsigned int> rgb32;
		if (m_enumRawType == RT_REAL)
		{
			const size_t sz = v.size()/2;
			if (!sz)
				return rgb32;
			for (size_t i = 0;i<sz;++i)
			{
				double val = v[i];
				if (val<m_dDBBottom)
					val = m_dDBBottom;
				if (val>m_dDBTop)
					val = m_dDBTop;
				double dc = (val-m_dDBBottom)/(m_dDBTop-m_dDBBottom);
				int r = 0, g = 0, b = 0;
				if (dc<0.33333)
					b = dc/.33333 * 255;
				else if (dc<0.66666)
				{
					b = 255 - (dc-0.33333)/.3*255;
					r = (dc-0.33333)/.3*255;
				}
				else {
					b = 0;
					r = 255;
					g =(dc-0.66667)/.3*255;
				}
				rgb32.push_back((r<<16)+(g<<8)+(b));
			}

		}
		else if  (m_enumRawType == RT_COMPLEX)
		{
			const size_t sz = v.size();
			if (!sz)
				return rgb32;
			for (size_t i = 0;i<sz;++i)
			{
				double val = v[i];
				if (val<m_dDBBottom)
					val = m_dDBBottom;
				if (val>m_dDBTop)
					val = m_dDBTop;
				double dc = (val-m_dDBBottom)/(m_dDBTop-m_dDBBottom);
				int r = 0, g = 0, b = 0;
				if (dc<0.33333)
					b = dc/.33333 * 255;
				else if (dc<0.66666)
				{
					b = 255 - (dc-0.33333)/.3*255;
					r = (dc-0.33333)/.3*255;
				}
				else {
					b = 0;
					r = 255;
					g =(dc-0.66667)/.3*255;
				}
				rgb32.push_back((b<<16)+(g<<8)+(r));
			}
		}
		return rgb32;
	}
	double spectroGramFFT::raw_pt_to_coord(const long long v) const
	{
		if (fabs(sampleRate())<1e-9)
			return 0;
		return v / sampleRate();
	}
	long long spectroGramFFT::raw_coord_to_pt(const double v) const
	{
		return v * sampleRate() + .5;
	}
	double spectroGramFFT::trans_pt_to_coord(const long long v) const
	{
		if (!m_nTransSize)
			return 0;
		return v / m_nTransSize * m_dSampleRate;
	}
	long long spectroGramFFT::trans_coord_to_pt(const double v) const
	{
		if (fabs(sampleRate())<1e-9)
			return 0;
		return v * m_nTransSize / m_dSampleRate + .5;
	}

	void spectroGramFFT::appendBuf(const short * p, const size_t sz)
	{
		size_t szB = m_vecRawBuf.size();
		if (szB+sz >= m_maxRawSize)
		{
			if (szB <=sz)
			{
				m_vecRawBuf.clear();
				m_currBufStart += szB;
			}
			else
			{
				m_vecRawBuf.erase(m_vecRawBuf.begin(),m_vecRawBuf.begin()+sz);
				m_currBufStart += sz;
			}
		}
		std::copy(p,p+sz,std::back_inserter(m_vecRawBuf));
	}
}
