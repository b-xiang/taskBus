#ifndef LISTENTHREAD_H
#define LISTENTHREAD_H
#include <QByteArray>
#include <QObject>
#include <QThread>
class reciv_thread: public QThread{
	Q_OBJECT
public:
	explicit reciv_thread(QObject * parent);
	void run() override;
signals:
	void new_package(QByteArray arr);
	void sig_quit();
};

class send_object: public QObject{
	Q_OBJECT
public:
	explicit send_object(QObject * parent);
public slots:
	void send_package(QByteArray arr);
};


#endif
