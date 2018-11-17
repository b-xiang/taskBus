#ifndef DIALOGPLOTS_H
#define DIALOGPLOTS_H

#include <QDialog>
#include <QByteArray>
#include <QMap>
#include <QVector>
#include <QStandardItemModel>
#include "listen_thread.h"
#include "cmdlineparser.h"
namespace Ui {
	class DialogSQL;
}

class DialogSQL : public QDialog
{
	Q_OBJECT

public:
	explicit DialogSQL(const TASKBUS::cmdlineParser * ps, QWidget *parent = 0);
	~DialogSQL();
private:
	const TASKBUS::cmdlineParser * m_pCmd = nullptr;
	Ui::DialogSQL *ui;
	reciv_thread * m_rthread = nullptr;
	QStandardItemModel * m_pMod = nullptr;
private slots:
	void deal_message(QString);
};

#endif // DIALOGPLOTS_H
