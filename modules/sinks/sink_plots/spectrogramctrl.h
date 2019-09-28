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
private:
	Ui::spectroGramCtrl *ui;
	SPECGRAM_CORE::spectroGramFFT m_spfft;
	double m_lineSeconds = 0.1;
	int m_currTopLine = 0;
	long long m_totalTime = 0;
	QImage m_image;
public slots:
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
