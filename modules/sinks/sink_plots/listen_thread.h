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
	void new_textcmd(QString str);
	void sig_quit();
};


#endif
