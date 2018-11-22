#include "watchmemmodule.h"

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
	return 3;
}

QVariant WatchMemModule::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role==Qt::DisplayRole)
	{
		if (orientation==Qt::Horizontal)
		{
			if (section==0)	return tr("PID");
			else if (section==1)	return tr("Name");
			else if (section==2)	return tr("Memory");
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
		if (r<0 || r>m_info.size() || c>=3 || c<0)
			return QVariant();
		if (c==0)
			return m_info[r].pid;
		else if (c==1)
			return m_info[r].m_name;
		else
			return m_info[r].m_memsize/1024.0/1024.0;
	}
	return QVariant();
}


void WatchMemModule::update_items()
{
	beginResetModel();
	tb_watch_dog()->update_table();
	m_info = tb_watch_dog()->get_info();
	endResetModel();
}
