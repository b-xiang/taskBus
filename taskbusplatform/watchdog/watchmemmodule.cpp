#include "watchmemmodule.h"
#include <QDebug>
WatchMemModule::WatchMemModule(QObject *parent) : QAbstractTableModel(parent)
{

}
int WatchMemModule::rowCount(const QModelIndex &idx ) const
{
	if (idx.isValid()==true)
		return 0;
	return m_info.size();
}

int WatchMemModule::columnCount(const QModelIndex &idx ) const
{
	if (idx.isValid()==true)
		return 0;
	if (idx.parent().isValid()==true)
		return 0;
	return 7;
}

QVariant WatchMemModule::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role==Qt::DisplayRole)
	{
		if (orientation==Qt::Horizontal)
		{
			static const QString cnames[7] = {tr("PID"),tr("Name"),tr("Memory"),
									 tr("pack_reci"),tr("pack_sent"),tr("bytes_reci"),tr("bytes_sent")
									 };
			if (section && section<7)
				return cnames[section];
		}
		else if (orientation==Qt::Vertical)
		{
			QString strV;
			strV.sprintf("%d",section);
			return strV;
		}

	}
	return QAbstractTableModel::headerData(section,orientation,role);
}

QVariant WatchMemModule::data(const QModelIndex &index, int role ) const
{
	if (index.isValid()==false)
		return QVariant();
	if (index.parent().isValid()==true)
		return QVariant();
	if (role==Qt::DisplayRole)
	{
		int r = index.row();
		int c = index.column();

		if (r<0 || r>m_info.size())
			return QVariant();
		qint64 id = m_info[r].phandle;
		switch (c)
		{
		case 0:
			return m_info[r].pid;
		case 1:
			return m_info[r].m_name;
		case 2:
			return m_info[r].m_memsize/1024.0/1024.0;
		case 3:
			return m_map_ps[id].pr;
		case 4:
			return m_map_ps[id].ps;
		case 5:
			return m_map_ps[id].br;
		case 6:
			return m_map_ps[id].bs;
		default:
			break;
		}
	}
	return QVariant();
}


void WatchMemModule::update_items()
{
	beginResetModel();
	tb_watch_dog().update_table();
	m_info = tb_watch_dog().get_info();
	endResetModel();
}
void WatchMemModule::slot_packio(qint64 pid, quint64 pr, quint64 ps, quint64 br, quint64 bs)
{
	qDebug()<<pid<<","<<pr<<","<<ps<<","<<br<<","<<bs;
	m_map_ps[pid].pr = pr;
	m_map_ps[pid].br = br;
	m_map_ps[pid].ps = ps;
	m_map_ps[pid].bs = bs;
}
