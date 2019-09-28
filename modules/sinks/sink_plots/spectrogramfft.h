#ifndef SPECTROGRAMFFT_H
#define SPECTROGRAMFFT_H
#include "spectrogramcore.h"
#include "fftw3.h"
#include <vector>
namespace SPECGRAM_CORE {
	class spectroGramFFT : public spectroGramCore<short,double,unsigned int>
	{
	public:
		spectroGramFFT();
		~spectroGramFFT() override;
	protected:
		virtual void reset() override;
		virtual std::vector<short> fetch_raw(long long centerPoint) override;
		virtual std::vector<double> transform(const std::vector<short> & raw) override;
		virtual std::vector<unsigned int> trans_value_to_rgb(const std::vector<double> & v) const  override;
	public:
		virtual double raw_pt_to_coord(const long long v) const  override;
		virtual long long raw_coord_to_pt(const double v) const  override;
		virtual double trans_pt_to_coord(const long long v) const override;
		virtual long long trans_coord_to_pt(const double v) const  override;
	public:
		double sampleRate()const {return m_dSampleRate;}
		void setSampleRate(double s){m_dSampleRate = s;}
		double DBBottom()const  {return m_dDBBottom;}
		void setDBBottom(double s){m_dDBBottom = s;}
		double DBTop() const {return m_dDBTop;}
		void setDBTop(double s){m_dDBTop = s;}
	private:
		fftw_complex * m_out = nullptr;
		fftw_complex * m_in = nullptr ;
		fftw_plan m_plan = nullptr;
	private:
		//sample rate
		double m_dSampleRate;
		//dark bottom
		double m_dDBBottom = -40;
		double m_dDBTop = 120;
	public:
		void appendBuf(const short * p, const size_t sz);
		long long bufStart() const {return m_currBufStart;}
		long long bufEnd() const {return m_currBufStart +m_vecRawBuf.size();}
	private:
		//Data buffer
		long m_maxRawSize = 128*1024*1024;
		std::vector<short> m_vecRawBuf;
		long long m_currBufStart = 0;
	};
}


#endif // SPECTROGRAMFFT_H
