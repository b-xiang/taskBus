#ifndef WATCHMEMMODULE_H
#define WATCHMEMMODULE_H

#include <QAbstractTableModel>
#include "tbwatchdog.h"
class WatchMemModule : public QAbstractTableModel
{
	Q_OBJECT
public:
	explicit WatchMemModule(QObject *parent = nullptr);
	int rowCount(const QModelIndex &idx = QModelIndex()) const override;
	int columnCount(const QModelIndex &idx = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
public slots:
	void update_items();

private:
	QVector<TASKBUS::tagMemoryInfo> m_info;
};

#endif // WATCHMEMMODULE_H
