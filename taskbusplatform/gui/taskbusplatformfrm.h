#ifndef PROGRESSBUSPLATFORMFRM_H
#define PROGRESSBUSPLATFORMFRM_H
#include "taskmodule.h"
#include <QMainWindow>
#include <QMdiSubWindow>
#include <QStandardItemModel>
#include <QLabel>
#include <QMap>
#include <QSystemTrayIcon>
namespace Ui {
	class taskBus;
}

class taskBusPlatformFrm : public QMainWindow
{
	Q_OBJECT

public:
	explicit taskBusPlatformFrm(QWidget *parent = 0);
	~taskBusPlatformFrm();
private slots:
	void slot_showPropModel(QObject * objModel);
	void slot_showMsg(QString);
	void slot_openprj(QString);
	void slot_projstarted();
	void slot_projstopped();
	void slot_projclosed(QString fm);
	void on_action_Load_Module_triggered();
	void on_action_New_Project_triggered();
	void on_action_About_triggered();
	void on_action_Start_project_triggered();
	void on_action_Save_Project_triggered();
	void on_action_stop_project_triggered();
	void on_action_Open_Project_triggered();
	void on_mdiArea_subWindowActivated(QMdiSubWindow *arg1);
	void on_comboBox_class_currentIndexChanged(int index);

	void on_actionhideWindow_toggled(bool arg1);

protected:
	void timerEvent(QTimerEvent *event) override;	
	void closeEvent(QCloseEvent * event) override;
	void load_modules(QStringList lstNames);
	void load_default_modules();
	void save_default_modules();
private:
	Ui::taskBus *ui;
	QSystemTrayIcon * m_pTrayIcon = nullptr;
	QIcon m_iconTray[2];
	QLabel * m_pStatus;
	int m_nTmid = -1;
	QMap<QString,taskModule *> m_toolModules;
	QMap<QString,QMdiSubWindow *> m_activePagesFileName;	
	QStandardItemModel * m_pMsgModel;
	QStandardItemModel * m_pClassModel;
	QString inifile();
	static int m_doc_ins;

};

#endif // PROGRESSBUSPLATFORMFRM_H
