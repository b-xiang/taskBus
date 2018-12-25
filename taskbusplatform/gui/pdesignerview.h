/*!
 * PDesignerView provides  a modular drag-and-drop interface like Simulink or
 * gnu-radio that can be used to implement (quasi-)real-time processing logic
 * on a general-purpose computer.
 *
 *@author goldenhawking@163.com
 * 2017-01-13
*/
#ifndef PDESIGNERVIEW_H
#define PDESIGNERVIEW_H

#include <QWidget>
#include <QMap>
#include <QVector>
#include <QSet>
#include <QThread>
#include "taskmodule.h"
#include "core/taskproject.h"
class TGraphicsTaskItem;
class taskNode;
class QGraphicsScene;
class QGraphicsLineItem;
class taskBusPlatformFrm;
namespace Ui {
	class PDesignerView;
}

class PDesignerView : public QWidget
{
	Q_OBJECT

public:
	explicit PDesignerView(taskBusPlatformFrm * pMainfrm,QWidget *parent = 0);
	~PDesignerView();
	void show_prop_page(QObject * model);
	bool modified() const {return m_bModified;}
	void set_modified(bool bmod = true);
	//拖放事件
	//Drag and drop events
protected:
	void dragEnterEvent(QDragEnterEvent *event) override;
	void dropEvent(QDropEvent * event) override;
	void closeEvent(QCloseEvent *) override;
public slots:
	//重新计算并更新连接
	//connect pins with sp lines
	void update_paths();
	void run();
	void stop();
	void zoomIn();
	void zoomOut();
	void drawAll();
signals:
	void sig_showProp(QObject * model);
	void sig_message(QString str);
	void sig_openprj(QString);
	void sig_updatePaths();
	void sig_projstarted();
	void sig_projstopped();
signals:
	void cmd_start_project();
	void cmd_stop_project(QThread * th);
private:
	//基础数据结构
	//GUI basic structure
	Ui::PDesignerView *ui;
	//进程对应的图形
	//Graphics for process corresponding
	QVector< TGraphicsTaskItem *> m_vec_gitems;
	QGraphicsScene * m_scene;
	//连线，在update_paths更新
	//lines, uodated in void update_paths();
	QVector<QGraphicsLineItem *> m_lines;
	//Subject Instance to pos and colors
	QMap< unsigned int, QVector<QPointF> > m_idx_in2pos;
	QMap< unsigned int, QVector<QPointF> > m_idx_out2pos;
	QMap< unsigned int, QVector<QColor> > m_idx_in2color;
	//工程,the Project
	taskProject * m_project = nullptr;
	QThread * m_pRunThread = nullptr;
	QString m_strFullFilename;
	//Mainframe
	taskBusPlatformFrm * m_pMainFrm = nullptr;
	static int m_nextCV ;
	bool m_bModified = false;
public:
	taskProject * project(){return m_project;}
	int selectedNode();
	void deleteNode(int);
	void debug_node(int,bool);
	bool is_debug_node(int node);
	bool is_running();
	void open_project(QString fm);
	QString fullFileName() const {return m_strFullFilename;}
	void setFullFileName(const QString & n){m_strFullFilename = n;}
	void addCell(QMimeData * data);
protected:
	void callbk_instanceAppended(taskCell * pmod, taskNode * pnod,QPointF pt);
	taskCell * callbk_newcell();
	QPointF callbk_GetCellPos(int ncell);
	void callbk_refreshIdx();
private slots:
	void slot_project_stopped();
	void slot_project_started();
	void on_actionzoom_In_triggered();
	void on_actionzoom_Out_triggered();
	void on_actionzoom_orgin_triggered();
	void on_actionDelete_selected_node_triggered();
	void on_actiondebug_on_triggered();
	void on_actiondebug_off_triggered();
	void on_actionCopy_triggered();
	void on_actionPaste_triggered();
	void on_actionCut_triggered();
	void on_actionConnectLine_triggered();
	void on_actionDeleteLine_triggered();
	void on_actionPinUp_triggered();
	void on_actionPinDown_triggered();
	void on_actionPinSide_triggered();
	void on_actionNiceUp_triggered();
	void on_actionNiceDown_triggered();
};

#endif // PDESIGNERVIEW_H
