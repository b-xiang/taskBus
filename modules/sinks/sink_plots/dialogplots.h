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
#include "spectrowidget.h"
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

	QVector<double> flush_data(QByteArray package);
private:
	const TASKBUS::cmdlineParser * m_cmd = nullptr;
	Ui::DialogPlots *ui;
	reciv_thread * m_rthread = nullptr;
	QMap<quint64,int> m_subidxs;
	QVector<QChartView *> m_chat_views;
	QVector<QValueAxis *> m_char_axis_x;
	QVector<QValueAxis *> m_char_axis_y;
	QVector<QXYSeries *> m_chat_serials;
	QVector<SpectroWidget *> m_chat_spec;
	QVector<QChart *> m_chars;
	QMap<int,int> m_plot_idxes;
	QVector<int> m_plot_chans;
	QVector<int> m_plot_types;

	QMap<quint64,QVector<double> > m_plot_buffer;
	QMap<quint64,bool > m_plot_refresh;

	int tid = -1;
	int refid = -1;
private slots:
	void deal_package(QByteArray);
	void on_pushButton_reset_clicked();
};

#endif // DIALOGPLOTS_H
