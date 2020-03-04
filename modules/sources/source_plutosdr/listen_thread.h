#ifndef LISTEN_THREAD_H
#define LISTEN_THREAD_H

#include <QThread>

class listen_thread: public QThread
{
	Q_OBJECT
public:
	explicit listen_thread(QObject * parent);
protected:
	void run() override;
signals:
	void quit_app();
};

#endif // LISTEN_THREAD_H
