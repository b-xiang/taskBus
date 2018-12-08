#include "dialogplots.h"
#include "ui_dialogplots.h"
#include "tb_interface.h"
#include <algorithm>
DialogPlots::DialogPlots(const TASKBUS::cmdlineParser * cmd,QWidget *parent) :
	QDialog(parent),
	m_cmd(cmd),
	ui(new Ui::DialogPlots),
	m_rthread(new reciv_thread(this))
{
	ui->setupUi(this);
	connect(m_rthread,&reciv_thread::sig_quit,this,&DialogPlots::close);
	connect(m_rthread,&reciv_thread::new_package,this,&DialogPlots::deal_package);
	connect(m_rthread,&reciv_thread::new_textcmd,ui->lineEdit_messages,&QLineEdit::setText);
	Qt::WindowFlags flg = windowFlags();
	flg |= Qt::WindowMinMaxButtonsHint;
	setWindowFlags(flg);
	m_rthread->start();

	//命令行参数解释
	QString schannels = QString::fromStdString(m_cmd->toString("channels","1"));
	QStringList lst = schannels.split(",");
	foreach(QString c, lst)
	{
		int vc = c.toInt();
		if (vc!=1 && vc !=2)
			vc = 1;
		m_plot_chans.push_back(vc);
	}
	QMap<int,int> plot_idxes;
	for (int i=0;i<16;++i)
	{
		QString key = QString("plot%1").arg(i);
		int ins = m_cmd->toInt(key.toStdString(),0);
		if (ins!=0)
			m_plot_idxes[ins] = i;
	}
}

DialogPlots::~DialogPlots()
{
	m_rthread->terminate();
	m_rthread->wait();
	delete ui;
}

void DialogPlots::deal_package(QByteArray package)
{
	using namespace TASKBUS;
	if (!m_cmd)
		return;

	const subject_package_header * pheader = (const subject_package_header *)
			package.constData();
	if (m_plot_idxes.contains(pheader->subject_id)==false)
		return;

	const int nSub = m_plot_idxes[pheader->subject_id];
	const int channels = nSub<m_plot_chans.size()?m_plot_chans[nSub]:1;

	const int pts = pheader->data_length/sizeof(double)/channels;
	const double * fdata =  (const double *)(package.constData()+sizeof(subject_package_header));
	if (pts<1)
		return;


	const quint64 hash_subidx = ((quint64(pheader->subject_id))<<32)+quint64(pheader->path_id);

	if (m_subidxs.contains(hash_subidx)==false)
	{
		if (tid==-1)
			tid=startTimer(1000);
		QChartView * pv = new QChartView(this);
		m_chat_views.push_back(pv);
		m_subidxs[hash_subidx] = m_chat_views.size()-1;

		QXYSeries * serials = nullptr;
		if (channels==1)
		{
			serials = new QLineSeries(this);
		}
		else
		{
			QScatterSeries * nserials = new QScatterSeries(this);
			nserials->setMarkerShape(QScatterSeries::MarkerShapeRectangle);
			nserials->setMarkerSize(1);
			serials = nserials;
		}
		m_chat_serials.push_back(serials);

		QValueAxis * ax = new QValueAxis(this);
		ax->setGridLineVisible(true);
		m_char_axis_x.push_back(ax);
		QValueAxis * ay = new QValueAxis(this);
		m_char_axis_y.push_back(ay);
		ay->setGridLineVisible(true);

		QChart *chart = new QChart();
		chart->legend()->hide();
		chart->addAxis(ax,Qt::AlignBottom);
		chart->addAxis(ay,Qt::AlignLeft);
		m_chars.push_back(chart);

		chart->setTheme(QChart::ChartThemeDark);

		pv->setChart(chart);
		chart->setTitle(QString("SUB%1 PATH%2").arg(pheader->subject_id).arg(pheader->path_id));

		pv->setRubberBand(QChartView::RectangleRubberBand);

		int rc = 0;
		int cc = 0;
		int kt = 1;
		while (kt < m_chars.size())
		{
			++kt;
			if (rc < cc)
			{
				++rc;
				cc = 0;
			}
			else
			{
				++cc;
				//rc = 0;
			}
		}

		chart->addSeries(serials);
		serials->attachAxis(ax);
		serials->attachAxis(ay);


		pv->show();
		ui->gridLayout->addWidget(pv,rc,cc);
	}
	const int subid = m_subidxs[hash_subidx];
	QXYSeries * serials = m_chat_serials[subid];
	double max_x = 0, max_y =  fdata[0];
	double min_x = 0, min_y =  fdata[0];

	if (serials->points().size()>4096 || pts>1)
	{
		QVector<QPointF> vec_points;
		for (int i=0;i<pts;++i)
		{
			double x,y;
			if (channels==1)
				x = i,y=fdata[i];
			else
				x = fdata[i*2], y = fdata[i*2+1];
			vec_points.push_back(QPointF(x,y));

			if (x>max_x)					max_x = x;
			if (x<min_x)					min_x = x;
			if (y>max_y)					max_y = y;
			if (y<min_y)					min_y = y;

		}
		serials->replace(vec_points);
	}
	else
	{
		QList<QPointF> lstpts = serials->points();
		const int rawsz = serials->points().size();
		for (int i=0;i<pts;++i)
			lstpts.push_back(QPointF(i+rawsz,fdata[i]));

		foreach(QPointF p, lstpts)
		{
			double x = p.x(), y = p.y();
			if (x>max_x)					max_x = x;
			if (x<min_x)					min_x = x;
			if (y>max_y)					max_y = y;
			if (y<min_y)					min_y = y;

		}

		serials->replace(lstpts);

	}

	QChart *chart = m_chars[subid];
	QValueAxis * ax = m_char_axis_x[subid];
	QValueAxis * ay = m_char_axis_y[subid];

	const double seed_x = max_x - min_x;
	const double seed_y = max_y - min_y;
	double new_max_x = ax->max(), new_max_y = ay->max();
	double new_min_x = ax->min(), new_min_y = ay->min();

	bool needup_x = false,needup_y = false;

	if (max_x > new_max_x)	new_max_x = max_x + seed_x * 0.2,needup_x=true;
	if (max_x *1.2 < new_max_x)	new_max_x = max_x + seed_x * 0.2,needup_x=true;

	if (max_y > new_max_y)	new_max_y = max_y + seed_y * 0.2,needup_y=true;
	if (max_y *1.2 < new_max_y)	new_max_y = max_y + seed_y * 0.2,needup_y=true;

	if (min_x < new_min_x)	new_min_x = min_x,needup_x=true;
	if (min_x > new_min_x * 1.2)	new_min_x = min_x,needup_x=true;

	if (min_y < new_min_y)	new_min_y = min_y,needup_y=true;
	if (min_y > new_min_y * 1.2)	new_min_y = min_y,needup_y=true;


	if (ui->checkBox_auto_reser->isChecked())
	{
		if (needup_x==true)
			ax->setRange(new_min_x,new_max_x);
		if (needup_y==true)
			ay->setRange(new_min_y,new_max_y);

	}

}

void DialogPlots::on_pushButton_reset_clicked()
{
	const int nGs = m_chars.size();
	for (int subid=0;subid<nGs;++subid)
	{
		QXYSeries * serials = m_chat_serials[subid];
		QChart *chart = m_chars[subid];
		QValueAxis * ax = m_char_axis_x[subid];
		QValueAxis * ay = m_char_axis_y[subid];
		QChartView * pv = m_chat_views[subid];
		int oldsize = serials->points().size();
		if (oldsize)
		{
			double max_x = serials->at(0).x(), max_y = serials->at(0).y();
			double min_x = serials->at(0).x(), min_y = serials->at(0).y();
			for (int i=1;i<oldsize;++i)
			{
				double x = serials->at(i).x(), y = serials->at(i).y();
				if (x>max_x)					max_x = x;
				if (x<min_x)					min_x = x;
				if (y>max_y)					max_y = y;
				if (y<min_y)					min_y = y;
			}

			const double seed_x = max_x - min_x;
			const double seed_y = max_y - min_y;
			double new_max_x = ax->max(), new_max_y = ay->max();
			double new_min_x = ax->min(), new_min_y = ay->min();

			bool needup_x = false,needup_y = false;

			if (max_x > new_max_x)	new_max_x = max_x + seed_x * 0.2,needup_x=true;
			if (max_x *1.2 < new_max_x)	new_max_x = max_x + seed_x * 0.2,needup_x=true;

			if (max_y > new_max_y)	new_max_y = max_y + seed_y * 0.2,needup_y=true;
			if (max_y *1.2 < new_max_y)	new_max_y = max_y + seed_y * 0.2,needup_y=true;

			if (min_x < new_min_x)	new_min_x = min_x,needup_x=true;
			if (min_x > new_min_x * 1.2)	new_min_x = min_x,needup_x=true;

			if (min_y < new_min_y)	new_min_y = min_y,needup_y=true;
			if (min_y > new_min_y * 1.2)	new_min_y = min_y,needup_y=true;



			if (needup_x==true)
				ax->setRange(new_min_x,new_max_x);
			if (needup_y==true)
				ay->setRange(new_min_y,new_max_y);



		}

	}
}

void DialogPlots::timerEvent(QTimerEvent *event)
{
	if (event->timerId()==tid)
	{
		killTimer(tid);
		tid = -1;
		on_pushButton_reset_clicked();
	}
}
