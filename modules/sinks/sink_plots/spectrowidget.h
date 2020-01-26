#ifndef SPECTROWIDGET_H
#define SPECTROWIDGET_H

#include <QWidget>
#include <QImage>
class SpectroWidget : public QWidget
{
	Q_OBJECT
public:
	explicit SpectroWidget(QWidget *parent = nullptr);
protected:
	void paintEvent(QPaintEvent * evt);
public:
	void append_data(QVector<double> vec_data,
					 double left,
					 double right,
					 double left_x,
					 double right_x
					 );
protected:
	QImage m_image;
	double m_minBound;
	double m_maxBound;
};

#endif // SPECTROWIDGET_H
