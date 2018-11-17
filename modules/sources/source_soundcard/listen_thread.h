#ifndef LISTEN_THREAD_H
#define LISTEN_THREAD_H

#include <QThread>
#include <QAtomicInteger>
#include "cmdlineparser.h"
class listen_thread: public QThread
{
	Q_OBJECT
public:
	explicit listen_thread(const TASKBUS::cmdlineParser * cmdline, QObject * parent);
	static QAtomicInteger<quint64> m_refTms;
protected:
	const TASKBUS::cmdlineParser * m_cmd = nullptr;
	void run() override;
signals:
	void quit_app();
};

#endif // LISTEN_THREAD_H
