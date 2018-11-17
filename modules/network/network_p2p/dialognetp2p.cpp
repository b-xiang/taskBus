#include "dialognetp2p.h"
#include "ui_dialognetp2p.h"
#include <QDateTime>
#include <QMessageBox>
#include <QSettings>
#include <QTextStream>
#include "tb_interface.h"
DialogNetP2P::DialogNetP2P(QWidget *parent)
	:QDialog(parent)
	,ui(new Ui::DialogNetP2P)
	,m_svr( new QTcpServer(this))
	,m_pRevThd(new reciv_thread(this))
{
	ui->setupUi(this);
	connect(m_svr,&QTcpServer::newConnection,this,&DialogNetP2P::slot_new_connection);
	connect(m_pRevThd,&reciv_thread::new_package,this,&DialogNetP2P::slot_new_taskpack,Qt::QueuedConnection);
	connect(m_pRevThd,&reciv_thread::sig_quit,this,&DialogNetP2P::close);
	m_pRevThd->start();
}

DialogNetP2P::~DialogNetP2P()
{
	m_pRevThd->terminate();
	delete ui;
}



void  DialogNetP2P::timerEvent(QTimerEvent * e)
{
	if (e->timerId()==m_nTimer)
	{
		if (m_n_mod)
		{
			if (m_sock)
			{
				if (m_lastState!=m_sock->state())
				{
					fprintf(stderr,"Socket State: %d->%d\n", m_lastState,m_sock->state());
					fflush(stderr);
					m_lastState = m_sock->state();
				}
				if(m_sock->state()!=QTcpSocket::ConnectedState &&
						m_sock->state()!=QTcpSocket::ConnectingState &&
						m_sock->state()!=QTcpSocket::HostLookupState
						)
				{
					m_sock->abort();
					m_sock->connectToHost(QHostAddress(m_str_addr),m_n_port);
				}

			}
		}
		else
		{
			if (m_svr->isListening()==false)
			{
				if (true==m_svr->listen(QHostAddress(m_str_addr),m_n_port))
				{
					fprintf(stderr,"Listening on port %d\n",m_n_port);
					fflush(stderr);
				}
				else
				{
					fprintf(stderr,"Can not open port %d\n",m_n_port);
					fflush(stderr);
				}

			}


		}
	}
	return QDialog::timerEvent(e);
}

void DialogNetP2P::slot_new_connection()
{
	while (m_svr->hasPendingConnections())
	{
		QTcpSocket * sock = m_svr->nextPendingConnection();
		if (m_sock)
		{
			m_sock->abort();
			m_sock->deleteLater();
		}
		m_sock = sock;
		connect(sock,&QTcpSocket::readyRead,this,&DialogNetP2P::slot_read_sock);
	}
}

void DialogNetP2P::setNetPara(const QString addr, const int port, const int mod)
{
	m_str_addr = addr;
	m_n_port = port;
	m_n_mod = mod;
	ui->lineEdit_listenerAddr->setText(m_str_addr);
	ui->lineEdit_listenerPort->setText(QString("%1").arg(port));
	ui->checkBox_listener_positive->setChecked(mod==0?true:false);
}

void DialogNetP2P::setMap(const QMap<int,int> mpi, const QVector<int> mpo)
{
	m_map_ins2inport = mpi;
	m_vec_outport2ins = mpo;
}

void DialogNetP2P::on_pushButton_listerner_start_clicked()
{
	startWork();
}

void DialogNetP2P::startWork()
{
	ui->pushButton_listerner_start->setDisabled(true);
	if (m_nTimer<0)
	{
		if (m_n_mod==0)
		{
			if (m_svr->isListening())
				m_svr->close();
			if (true==m_svr->listen(QHostAddress(m_str_addr),m_n_port))
			{
				fprintf(stderr,"Listening on port %d\n",m_n_port);
				fflush(stderr);
			}
			else
			{
				fprintf(stderr,"Can not open port %d\n",m_n_port);
				fflush(stderr);
			}
		}
		else
		{
			if (m_sock)
			{
				m_sock->abort();
				m_sock->deleteLater();
			}
			m_sock = new QTcpSocket(this);
			m_sock->connectToHost(QHostAddress(m_str_addr),m_n_port);
			connect(m_sock,&QTcpSocket::readyRead,this,&DialogNetP2P::slot_read_sock);
			fprintf(stderr,"connecting to: %s:%d\n", m_str_addr.toStdString().c_str(), m_n_port);
			fflush(stderr);
		}
		m_nTimer=startTimer(1000);
	}
}

void DialogNetP2P::on_checkBox_listener_positive_stateChanged(int arg1)
{
	m_n_mod = ui->checkBox_listener_positive->isChecked()?0:1;
}

void DialogNetP2P::on_lineEdit_listenerAddr_textChanged(const QString &arg1)
{
	m_str_addr = ui->lineEdit_listenerAddr->text();
}

void DialogNetP2P::on_lineEdit_listenerPort_textChanged(const QString &arg1)
{
	m_n_port = ui->lineEdit_listenerPort->text().toInt();
}



void DialogNetP2P::slot_read_sock()
{
	using namespace TASKBUS;
	QByteArray arrData = m_sock->readAll();
	m_package_array.append(arrData);
	while (m_package_array.size()>=sizeof(subject_package_header))
	{
		//检查独特码
		int goodoff = 0;
		while (!(m_package_array[0+goodoff]==0x3c && m_package_array[1+goodoff]==0x5A
				 &&m_package_array[2+goodoff]==0x7E  &&m_package_array[3+goodoff]==0x69 ))
		{
			++goodoff;
			if (goodoff+3>= m_package_array.size())
				break;
		}
		if (goodoff)
			m_package_array.remove(0,goodoff);
		if (m_package_array.size()<sizeof(subject_package_header))
			break;
		const subject_package_header * pheader = (const subject_package_header *)
				m_package_array.constData();

		Q_ASSERT(m_package_array[0+goodoff]==0x3c && m_package_array[1+goodoff]==0x5A
				&&m_package_array[2+goodoff]==0x7E  &&m_package_array[3+goodoff]==0x69 );


		if (pheader->data_length>256*1024*1024)
		{
			fprintf(stderr,"package too big: %d\n", pheader->data_length);
			fflush(stderr);
			m_package_array.clear();
			break;
		}
		if (m_package_array.size()<pheader->data_length+sizeof(subject_package_header))
			break;
		if (pheader->subject_id>=0 && pheader->subject_id<m_vec_outport2ins.size())
		{
			if (m_vec_outport2ins[pheader->subject_id]>0)
			{
				const char * dptr = m_package_array.constData()+sizeof(subject_package_header);
				push_subject(m_vec_outport2ins[pheader->subject_id],pheader->path_id,
						pheader->data_length,(const unsigned char *)dptr
						);
			}
		}
		m_package_array.remove(0,sizeof(subject_package_header)+pheader->data_length);
	}
}


void DialogNetP2P::slot_new_taskpack(QByteArray package)
{
	using namespace TASKBUS;
	const subject_package_header * pheader = (const subject_package_header *)
			package.constData();
	const int pts = pheader->data_length;
	const unsigned char * fdata =  (const unsigned char *)(package.constData()+sizeof(subject_package_header));
	if (pts<1)
		return;
	subject_package_header header = *pheader;

	if (m_map_ins2inport.contains(header.subject_id))
	{
		header.subject_id = m_map_ins2inport[header.subject_id];
		if(m_sock)
		{
			if (m_sock->state()==QTcpSocket::ConnectedState)
			{
				m_sock->write((const char *)&header,sizeof(subject_package_header));
				m_sock->write((const char *)fdata,header.data_length);
			}
		}

	}
}
