#include "dlgwrpscript.h"
#include "ui_dlgwrpscript.h"
#include <QSettings>
#include <QProcessEnvironment>
#include <QFileDialog>
#include "tb_interface.h"
using namespace TASKBUS;
DlgWrpScript::DlgWrpScript(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::DlgWrpScript),
	m_process(new QProcess(this))
{
	ui->setupUi(this);
	loadIni();
	connect(m_process,&QProcess::started,this,
			&DlgWrpScript::slot_started,Qt::QueuedConnection);
	connect(m_process,
			static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>
			(&QProcess::finished),
			this,&DlgWrpScript::slot_stopped,Qt::QueuedConnection);
	connect(m_process,&QProcess::readyReadStandardOutput,this,
			&DlgWrpScript::slot_readyReadStandardOutput,Qt::QueuedConnection);
	connect(m_process,&QProcess::readyReadStandardError,this,
			&DlgWrpScript::slot_readyReadStandardError,Qt::QueuedConnection);
	connect(m_process,&QProcess::bytesWritten,this,
			&DlgWrpScript::slot_sended,Qt::QueuedConnection);

}

DlgWrpScript::~DlgWrpScript()
{
	cmd_stop();
	if (m_plistenThd)
	{
		m_plistenThd->terminate();
		m_plistenThd->wait();
	}
	delete ui;
}

void DlgWrpScript::run()
{
	if (m_plistenThd==nullptr)
	{
		m_plistenThd = new listen_thread(this);
		connect(m_plistenThd,&listen_thread::quit_app,
				this,&DlgWrpScript::close);
		connect(m_plistenThd,&listen_thread::new_package,
				this,&DlgWrpScript::slot_newPackage
				);
		m_plistenThd->start();
	}

	if (m_timerId==-1)
		m_timerId = startTimer(200);

	on_pushButton_start_clicked();
}
bool DlgWrpScript::cmd_start()
{
	if (m_process->state()!=QProcess::NotRunning)
		return false;
	const QString cmdline = ui->lineEdit_prgPath->text();
	QStringList lstCmds;
	lstCmds << ui->lineEdit_parasec->text();
	lstCmds << m_lstArgs;
	m_process->setProgram(cmdline);
	//设置当前路径
	QDir::setCurrent(ui->lineEdit_workingDir->text());

	//准备环境变量
	QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
	const QString newEnv = ui->plainTextEdit_extraEnv->toPlainText();
	const QStringList newEnvPai = newEnv.split("\n");
	foreach(QString evs, newEnvPai)
	{
		const QString tvs = evs.trimmed();
		const QStringList lst_kv = tvs.split("=");
		if (lst_kv.size()!=2)
			continue;
		QString skey = lst_kv.first().trimmed();
		QString sval = lst_kv.last().trimmed();
		if (env.contains(skey))
		{
			QString oldval = env.value(skey);
			env.insert(skey,sval+";"+oldval);
		}
		else
			env.insert(skey,sval);
	}
	m_process->setProcessEnvironment(env);
	m_process->setArguments(lstCmds);
	m_process->start();
	return true;
}


bool DlgWrpScript::cmd_stop()
{
	if (m_process->state()!=QProcess::Running)
		return false;
	//发送信令，终止
	char cmd[] = "\"quit\":{ret = 0}";
	subject_package_header header;
	header.prefix[0] = 0x3C;
	header.prefix[1] = 0x5A;
	header.prefix[2] = 0x7E;
	header.prefix[3] = 0x69;
	header.data_length = strlen(cmd)+1;
	header.path_id = 1;
	header.subject_id = control_subect_id();
	m_process->write((char *)&header,sizeof(subject_package_header));
	m_process->write(cmd,strlen(cmd)+1);
	int c = 0;
	while (++c<100 && m_process->state()==QProcess::Running)
	{
		m_process->waitForFinished(20);
		QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
	}
	m_process->kill();
	return true;
}

void DlgWrpScript::slot_started()
{
	ui->pushButton_start->setText(tr("Stop"));
	ui->pushButton_start->setEnabled(true);

}

void DlgWrpScript::slot_stopped()
{
	ui->pushButton_start->setText(tr("Start"));
	ui->pushButton_start->setEnabled(true);

	this->show();

}

void DlgWrpScript::slot_readyReadStandardOutput()
{
	QByteArray arred = m_process->readAllStandardOutput();
	fwrite(arred.constData(),1,arred.size(),stdout);
	fflush(stdout);
}


void DlgWrpScript::slot_readyReadStandardError()
{
	QByteArray arred =m_process->readAllStandardError();
	fwrite(arred.constData(),1,arred.size(),stderr);
	fflush(stderr);

}

void DlgWrpScript::slot_sended(qint64 b)
{

}

void DlgWrpScript::slot_newPackage(QByteArray arr)
{
	m_buffer_pack.push_back(arr);
	if (isRunning())
	{
		while(m_buffer_pack.size())
		{
			m_process->write(m_buffer_pack.first());
			m_buffer_pack.pop_front();
		}
	}
}

bool DlgWrpScript::isRunning ()
{
	return m_process->state()==QProcess::NotRunning?false:true;
}

void DlgWrpScript::on_pushButton_start_clicked()
{
	saveIni();
	if (isRunning()==false)
		cmd_start();
	else
		cmd_stop();
}

void DlgWrpScript::timerEvent(QTimerEvent *event)
{
	static int ct = 0;
	if (event->timerId()==m_timerId)
	{
		if (isRunning())
		{
			while(m_buffer_pack.size())
			{
				m_process->write(m_buffer_pack.first());
				m_buffer_pack.pop_front();
			}
			ct = 0;
		}
		else
		{
			if (++ct % 10==0)
			{
				fprintf(stderr,"waiting for start...");
				fflush(stderr);
				if (this->isVisible()==false)
					this->show();
			}
			if (ct >= 300)
			{
				fprintf(stderr,"Failed to start");
				fflush(stderr);
				this->show();
			}
		}

	}
}

void DlgWrpScript::on_toolButton_path_clicked()
{
	QSettings settings(QCoreApplication::applicationFilePath()+".ini",QSettings::IniFormat);
	QString hist_prgpath = settings.value("history/prgpath","/").toString();
	QString dirt = QFileDialog::getOpenFileName(this,tr("Get Exec Path"),hist_prgpath);
	if (dirt.length())
	{
		ui->lineEdit_prgPath->setText(dirt);
		settings.setValue("history/prgpath",dirt);
	}
}

void DlgWrpScript::on_toolButton_workingDir_clicked()
{
	QSettings settings(QCoreApplication::applicationFilePath()+".ini",QSettings::IniFormat);
	QString hist_workingdir = settings.value("history/workingdir","/").toString();
	QString dirt = QFileDialog::getExistingDirectory(this,tr("GetPath"),hist_workingdir);
	if (dirt.length())
	{
		ui->lineEdit_workingDir->setText(hist_workingdir);
		settings.setValue("history/workingdir",dirt);
	}

}
void DlgWrpScript::loadIni()
{
	QSettings settings(QCoreApplication::applicationFilePath()+".ini",QSettings::IniFormat);
	QString lineEdit_parasec = settings.value("settings/lineEdit_parasec","/home/scrypt/myprog.py").toString();
	ui->lineEdit_parasec->setText(lineEdit_parasec);
	QString lineEdit_prgPath = settings.value("settings/lineEdit_prgPath","/usr/bin/python3").toString();
	ui->lineEdit_prgPath->setText(lineEdit_prgPath);
	QString lineEdit_workingDir = settings.value("settings/lineEdit_workingDir","/home/data").toString();
	ui->lineEdit_workingDir->setText(lineEdit_workingDir);
	QString plainTextEdit_extraEnv = settings.value("settings/plainTextEdit_extraEnv","").toString();
	ui->plainTextEdit_extraEnv->setPlainText( plainTextEdit_extraEnv);

}
void DlgWrpScript::saveIni()
{
	QSettings settings(QCoreApplication::applicationFilePath()+".ini",QSettings::IniFormat);

	QString lineEdit_parasec = ui->lineEdit_parasec->text();
	settings.setValue("settings/lineEdit_parasec",lineEdit_parasec);
	QString lineEdit_prgPath = ui->lineEdit_prgPath->text();
	settings.setValue("settings/lineEdit_prgPath",lineEdit_prgPath);
	QString lineEdit_workingDir = ui->lineEdit_workingDir->text();
	settings.setValue("settings/lineEdit_workingDir",lineEdit_workingDir);
	QString plainTextEdit_extraEnv = ui->plainTextEdit_extraEnv->toPlainText();
	settings.setValue("settings/plainTextEdit_extraEnv",plainTextEdit_extraEnv);
}
