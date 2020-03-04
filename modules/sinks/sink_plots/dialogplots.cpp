#include "dialogplots.h"
#include "ui_dialogplots.h"
#include "tb_interface.h"
#include <algorithm>
#include <QRandomGenerator64>

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
	QString schannels = QString::fromStdString(m_cmd->toString("channels","1,1,1,1,1"));
	QStringList lst = schannels.split(",");
	foreach(QString c, lst)
	{
		int vc = c.toInt();
		if (vc!=1 && vc !=2 && vc !=0)
			vc = 1;
		m_plot_chans.push_back(vc);
	}
	QString stypes = QString::fromStdString(m_cmd->toString("datatypes","9,9,9,9,9"));
	QStringList lsttps = stypes.split(",");
	foreach(QString c, lsttps)
	{
		int vc = c.toInt();
		if (vc<0 || vc>10)
			vc = 9;
		m_plot_types.push_back(vc);
	}
	QMap<int,int> plot_idxes;
	//Key is subject id, value is the id of plots
	for (int i=0;i<16;++i)
	{
		QString key = QString("plot%1").arg(i);
		int ins = m_cmd->toInt(key.toStdString(),0);
		if (ins!=0)
			m_plot_idxes[ins] = i;
	}

	refid = startTimer(30);
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
	const int nIDofPlot = m_plot_idxes[pheader->subject_id];
	const int channels = nIDofPlot<m_plot_chans.size()?m_plot_chans[nIDofPlot]:1;

	const quint64 hash_subidx = ((quint64(pheader->subject_id))<<32)+quint64(pheader->path_id);
	m_plot_refresh[hash_subidx] = true;

	if (m_subidxs.contains(hash_subidx)==false)
	{
		QString strName = tr("Sub%1 Path%2").arg(pheader->subject_id).arg(pheader->path_id);
		if (tid==-1)
			tid=startTimer(1000);
		QChartView * pv = new QChartView(this);
		m_chat_views.push_back(pv);
		m_subidxs[hash_subidx] = m_chat_views.size()-1;

		QXYSeries * serials = nullptr;
		if (channels<2)
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
		chart->setTitle(tr("SUB%1 PATH%2").arg(pheader->subject_id).arg(pheader->path_id));

		pv->setRubberBand(QChartView::RectangleRubberBand);

		chart->addSeries(serials);
		serials->attachAxis(ax);
		serials->attachAxis(ay);
		pv->show();
		if (channels==0)
		{
			QSplitter * spliter = new QSplitter(this);
			spliter->setOrientation(Qt::Vertical);
			spliter->addWidget(pv);
			SpectroWidget * ws = new SpectroWidget(this);
			spliter->addWidget(ws);
			ui->tabWidget_outputs->addTab(spliter,strName);
			m_chat_spec.push_back(ws);
			ws->show();
		}
		else
		{
			ui->tabWidget_outputs->addTab(pv,strName);
			m_chat_spec.push_back(0);
		}


	}

	m_plot_buffer[hash_subidx] = flush_data(package);
}

QVector<double> DialogPlots::flush_data(QByteArray package)
{
	QVector<double> vec_res;
	using namespace TASKBUS;
	if (!m_cmd)
		return vec_res;

	const subject_package_header * pheader = (const subject_package_header *)
			package.constData();

	if (m_plot_idxes.contains(pheader->subject_id)==false)
		return vec_res;
	const int nSub = m_plot_idxes[pheader->subject_id];
	const int rawchannels = nSub<m_plot_chans.size()?m_plot_chans[nSub]:1;
	const int channels = rawchannels<2?1:2;

	const int types = nSub<m_plot_types.size()?m_plot_types[nSub]:9;

	const char * rawdata =  (const char *)(package.constData()+sizeof(subject_package_header));
	int pts = 0;;

	//"range":{"value":"0:uint8 1:int8 2:uint16 3:int16 4:uint32 5:int32 6:uint64 7:int64 8:float 9:double"}
	switch(types)
	{
	case 0:
		pts = pheader->data_length/sizeof(quint8)/channels;
	{
		const quint8 * ptrRaw = (const quint8 *) rawdata;
		for (int i=0;i<pts;++i)
			vec_res<<ptrRaw[i];
	}
		break;
	case 1:
		pts = pheader->data_length/sizeof(qint8)/channels;
	{
		const qint8 * ptrRaw = (const qint8 *) rawdata;
		for (int i=0;i<pts;++i)
			vec_res<<ptrRaw[i];
	}
		break;
	case 2:
		pts = pheader->data_length/sizeof(quint16)/channels;
	{
		const quint16 * ptrRaw = (const quint16 *) rawdata;
		for (int i=0;i<pts;++i)
			vec_res<<ptrRaw[i];
	}
		break;
	case 3:
		pts = pheader->data_length/sizeof(qint16)/channels;
	{
		const qint16 * ptrRaw = (const qint16 *) rawdata;
		for (int i=0;i<pts;++i)
			vec_res<<ptrRaw[i];
	}
		break;
	case 4:
		pts = pheader->data_length/sizeof(quint32)/channels;
	{
		const quint32 * ptrRaw = (const quint32 *) rawdata;
		for (int i=0;i<pts;++i)
			vec_res<<ptrRaw[i];
	}
		break;
	case 5:
		pts = pheader->data_length/sizeof(qint32)/channels;
	{
		const qint32 * ptrRaw = (const qint32 *) rawdata;
		for (int i=0;i<pts;++i)
			vec_res<<ptrRaw[i];
	}
		break;
	case 6:
		pts = pheader->data_length/sizeof(quint64)/channels;
	{
		const quint64 * ptrRaw = (const quint64 *) rawdata;
		for (int i=0;i<pts;++i)
			vec_res<<ptrRaw[i];
	}
		break;
	case 7:
		pts = pheader->data_length/sizeof(qint64)/channels;
	{
		const qint64 * ptrRaw = (const qint64 *) rawdata;
		for (int i=0;i<pts;++i)
			vec_res<<ptrRaw[i];
	}
		break;
	case 8:
		pts = pheader->data_length/sizeof(float)/channels;
	{
		const float * ptrRaw = (const float *) rawdata;
		for (int i=0;i<pts;++i)
			vec_res<<ptrRaw[i];
	}
		break;
	case 9:
		pts = pheader->data_length/sizeof(double)/channels;
	{
		const double * ptrRaw = (const double *) rawdata;
		for (int i=0;i<pts;++i)
			vec_res<<ptrRaw[i];
	}
		break;
	default:
		break;
	}

	return vec_res;
}

void DialogPlots::on_pushButton_reset_clicked()
{
	const int nGs = m_chars.size();
	for (int subid=0;subid<nGs;++subid)
	{
		QXYSeries * serials = m_chat_serials[subid];
		//QChart *chart = m_chars[subid];
		QValueAxis * ax = m_char_axis_x[subid];
		QValueAxis * ay = m_char_axis_y[subid];
		//QChartView * pv = m_chat_views[subid];
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
	else if (event->timerId()==refid)
	{
		killTimer(refid);
		QRandomGenerator64 rg;
		QList<quint64> hash_subidxes = m_subidxs.keys();
		foreach (quint64 hash_subidx, hash_subidxes)
		{
			const int subject_id = hash_subidx>>32;
			if (!m_plot_idxes.contains(subject_id))
				continue;
			if (!m_plot_refresh[hash_subidx])
				continue;
			m_plot_refresh[hash_subidx] = false;
			const int nIDofPlot = m_plot_idxes[subject_id];
			const int rawchannels = nIDofPlot<m_plot_chans.size()?m_plot_chans[nIDofPlot]:1;
			const int subid = m_subidxs[hash_subidx];
			const int channels = rawchannels<2?1:2;
			QChartView * pv = m_chat_views[subid];
			QXYSeries * serials = m_chat_serials[subid];

			const int pts = m_plot_buffer[hash_subidx].size();
			const double * fdata = m_plot_buffer[hash_subidx].constData();

			double max_x = 0, max_y =  fdata[0];
			double min_x = 0, min_y =  fdata[0];

			int step = channels>1?(pts / 4096):(pts/65536);
			if (step<1)
				step = 1;


			if (serials->points().size()>4096 || pts>1)
			{
				SpectroWidget * ws = m_chat_spec[subid];
				QVector<QPointF> vec_points;
				for (int i=0;i<pts;i+=step)
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
				if (ws)
				{
					QChart * ca = m_chars[subid];
					QValueAxis * axx = m_char_axis_x[subid];
					double dmin = axx->min();
					double dmax = axx->max();
					QRectF r = ca->plotArea();


					ws->append_data(m_plot_buffer[hash_subidx],
									r.left(),
									r.right(),
									dmin,
									dmax
									);
				}

			}
			else
			{

				QList<QPointF> lstpts = serials->points();
				const int rawsz = serials->points().size();
				for (int i=0;i<pts;i+=step)
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

			//QChart *chart = m_chars[subid];
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
		refid = startTimer(40);
	}
}

