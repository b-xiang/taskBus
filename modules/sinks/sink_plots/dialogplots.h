#ifndef DIALOGPLOTS_H
#define DIALOGPLOTS_H

#include <QDialog>
#include <QByteArray>
#include <QMap>
#include <QVector>
#include <QtCharts>
#include <QStandardItemModel>
#include "cmdlineparser.h"
#include "tb_interface.h"
#include "listen_thread.h"

namespace Ui {
	class DialogPlots;
}

class DialogPlots : public QDialog
{
	Q_OBJECT

public:
	explicit DialogPlots(const TASKBUS::cmdlineParser * cmd,QWidget *parent = 0);
	~DialogPlots() override;
protected:
	void timerEvent(QTimerEvent *event) override;
private:
	const TASKBUS::cmdlineParser * m_cmd = nullptr;
	Ui::DialogPlots *ui;
	reciv_thread * m_rthread = nullptr;
	QMap<quint64,int> m_subidxs;
	QVector<QChartView *> m_chat_views;
	QVector<QValueAxis *> m_char_axis_x;
	QVector<QValueAxis *> m_char_axis_y;
	QVector<QXYSeries *> m_chat_serials;
	QVector<QChart *> m_chars;
	QMap<int,int> m_plot_idxes;
	QVector<int> m_plot_chans;
	int tid = -1;
	//FFT spec
	int m_nSpecFunc = 0;
	QStandardItemModel * m_pFFTSzMd;
private slots:
	void deal_package(QByteArray);
	void on_pushButton_reset_clicked();
	void on_comboBox_fft_currentIndexChanged(int index);
};

#endif // DIALOGPLOTS_H
