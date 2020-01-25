#include "spectrowidget.h"
#include <QPainter>
SpectroWidget::SpectroWidget(QWidget *parent)
	: QWidget(parent)
	, m_minBound(-120)
	, m_maxBound(0)
{

}
void SpectroWidget::paintEvent(QPaintEvent * evt)
{
	QPainter painter(this);

	QSize szWindows = size();
	if (!(szWindows.width()>16 && szWindows.height()>=16))
		return QWidget::paintEvent(evt);
	if (szWindows.width()!=m_image.width()||szWindows.height()!=m_image.height()||
			m_image.isNull())
	{
		m_image = QImage(szWindows,QImage::Format_RGB32);
		m_image.fill(QColor(0,0,0));
	}
	if (m_image.isNull())
		return QWidget::paintEvent(evt);


	painter.drawImage(0,0,m_image);
}

void SpectroWidget::append_data(QVector<double> vec_data)
{
	if (m_image.isNull())
		return;
	const int width = m_image.width();
	const int height = m_image.height();
	const int scsize = m_image.bytesPerLine();
	for (int l = height-1;l>0;--l)
		memcpy(m_image.scanLine(l),m_image.scanLine(l-1),scsize);

	const int datasz = vec_data.size();

	for (int r = 0;r<width;++r)
	{
		int from = r*1.0/width*datasz+.5;
		if (from<0) from = 0;
		if (from>=datasz) from = datasz -1;
		const double v = vec_data[from];
		const double vii = (v-m_minBound)/(m_maxBound-m_minBound);
		int red = 0,green = 0,blue = 0;
		if (vii>=0 && vii<0.25)
			blue = vii/0.25*255;
		else if (vii>=0.25 && vii<0.5 )
		{
			blue = (0.5-vii)/0.25*255;
			red =  (vii-0.25)/0.25*255;
		}
		else if (vii>=0.5 && vii<0.75 )
		{
			red =  255;
			green = (vii-0.5)/0.25*255;
		}
		else if (vii>=0.75 && vii<1)
		{
			red = 255;
			green = 255;
			blue =  (vii-0.75)/0.25*255;
		}
		else if (vii>=1)
			red = 255, blue = 255, green = 255;

		m_image.setPixel(r,0,qRgb(red,green,blue));
	}

	update();
}
