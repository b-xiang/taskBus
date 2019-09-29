#ifndef SPECTROGRAMCTRL_H
#define SPECTROGRAMCTRL_H

#include <QWidget>
#include <QImage>
namespace Ui {
	class spectroGramCtrl;
}
#include "spectrogramfft.h"
class spectroGramCtrl : public QWidget
{
	Q_OBJECT

public:
	explicit spectroGramCtrl(QWidget *parent = nullptr);
	~spectroGramCtrl() override;
	void paintEvent(QPaintEvent *event) override;

	double lineOffset() const {return m_lineOffset;}
	double sampleRate() const {return m_spfft.sampleRate();}

public:
	SPECGRAM_CORE::spectroGramFFT m_spfft;
private:
	Ui::spectroGramCtrl *ui;
	double m_lineSeconds = 0.1;
	double m_lineOffset = 0;
	int m_currTopLine = 0;
	long long m_totalTime = 0;
	QImage m_image;
public slots:
	void setLineSeconds(double s);
	void setLineOffset(double of);
	void setSampleRate(double v){
		m_spfft.setSampleRate(v);
		update();
	}
	void setTransSize(int v){
		m_spfft.setDataPara(v);
		update();
	}
	void append(const short * p, const size_t sz);
};

#endif // SPECTROGRAMCTRL_H
