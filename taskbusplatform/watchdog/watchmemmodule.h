#ifndef WATCHMEMMODULE_H
#define WATCHMEMMODULE_H

#include <QAbstractTableModel>
#include <QMap>
#include "tbwatchdog.h"
class WatchMemModule : public QAbstractTableModel
{
	Q_OBJECT
private:
	struct stat{
		quint64 ps;
		quint64 pr;
		quint64 bs;
		quint64 br;
	};
public:
	explicit WatchMemModule(QObject *parent = nullptr);
	int rowCount(const QModelIndex &idx = QModelIndex()) const override;
	int columnCount(const QModelIndex &idx = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
public slots:
	void update_items();
	void slot_packio(qint64 pid, quint64 pr, quint64 ps, quint64 br, quint64 bs);

private:
	QVector<TASKBUS::tagMemoryInfo> m_info;
	QMap<qint64, stat> m_map_ps;
};

#endif // WATCHMEMMODULE_H
