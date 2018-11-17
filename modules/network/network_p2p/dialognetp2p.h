#ifndef DIALOGNETP2P_H
#define DIALOGNETP2P_H

#include <QDialog>
#include <QSet>
#include <QStandardItemModel>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include <QVector>
#include "listen_thread.h"
namespace Ui {
	class DialogNetP2P;
}

class DialogNetP2P : public QDialog
{
	Q_OBJECT

public:
	explicit DialogNetP2P(QWidget *parent = 0);
	~DialogNetP2P();
	void setNetPara(const QString addr, const int port, const int mod);
	void setMap(const QMap<int,int> mpi, const QVector<int> mpo);
	void startWork();
protected:
	void timerEvent(QTimerEvent *event);

private:
	Ui::DialogNetP2P *ui = nullptr;

	QTcpServer * m_svr = nullptr;
	QTcpSocket * m_sock = nullptr;

	QString m_str_addr = "127.0.0.1";
	int	m_n_port = 9527;
	int m_n_mod = 0;

	QMap<int,int> m_map_ins2inport;
	QVector<int> m_vec_outport2ins;

	int m_nTimer = -1;

	reciv_thread * m_pRevThd = nullptr;

	QTcpSocket::SocketState m_lastState = QTcpSocket::UnconnectedState;

	QByteArray m_package_array;


protected slots:
	void slot_new_connection();
	void slot_read_sock();
	void slot_new_taskpack(QByteArray);
private slots:
	void on_pushButton_listerner_start_clicked();
	void on_checkBox_listener_positive_stateChanged(int arg1);
	void on_lineEdit_listenerAddr_textChanged(const QString &arg1);
	void on_lineEdit_listenerPort_textChanged(const QString &arg1);
};

#endif // DIALOGNETP2P_H
