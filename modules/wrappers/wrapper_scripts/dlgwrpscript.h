#ifndef DLGWRPSCRIPT_H
#define DLGWRPSCRIPT_H

#include <QDialog>
#include <QProcess>
#include <QList>
#include "listen_thread.h"
namespace Ui {
	class DlgWrpScript;
}

class DlgWrpScript : public QDialog
{
	Q_OBJECT

public:
	explicit DlgWrpScript(QWidget *parent = nullptr);
	~DlgWrpScript();
	QStringList m_lstArgs;
	bool		isRunning ();
	QProcess *	proc(){return m_process;}
protected:
	void timerEvent(QTimerEvent *event);
public slots:
	bool cmd_start();
	bool cmd_stop();
	void run();
private slots:
	void slot_newPackage(QByteArray arr);
	void slot_readyReadStandardOutput();
	void slot_readyReadStandardError();
	void slot_sended(qint64 );
	void slot_started( );
	void slot_stopped();
	void on_pushButton_start_clicked();
	void on_toolButton_path_clicked();
	void on_toolButton_workingDir_clicked();
private:
	Ui::DlgWrpScript *ui;
	QList<QByteArray> m_buffer_pack;
	int m_timerId = -1;
	listen_thread * m_plistenThd = nullptr;
	QProcess * m_process = nullptr;
	void loadIni();
	void saveIni();
};

#endif // DLGWRPSCRIPT_H
