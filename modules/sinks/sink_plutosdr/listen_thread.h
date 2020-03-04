#ifndef LISTEN_THREAD_H
#define LISTEN_THREAD_H

#include <QThread>
#include "../tb_interface/cmdlineparser.h"
class listen_thread: public QThread
{
	Q_OBJECT
public:
	explicit listen_thread(const TASKBUS::cmdlineParser & a,QObject * parent);
protected:
	void run() override;
	const TASKBUS::cmdlineParser & args;
signals:
	void quit_app();
};

#endif // LISTEN_THREAD_H
