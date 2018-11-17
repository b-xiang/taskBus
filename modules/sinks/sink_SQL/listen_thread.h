#ifndef LISTENTHREAD_H
#define LISTENTHREAD_H
#include <QByteArray>
#include <QObject>
#include <QThread>
#include "cmdlineparser.h"
class reciv_thread: public QThread{
	Q_OBJECT
public:
	explicit reciv_thread(const TASKBUS::cmdlineParser * m_pCmd,QObject * parent);
	void run() override;
signals:
	void new_message(QString arr);
	void err_message(QString err);
	void sig_quit();
private:
	const TASKBUS::cmdlineParser * m_pCmd = nullptr;
	int m_nTotalOK = 0;


};


#endif
