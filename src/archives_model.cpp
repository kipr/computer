#include "archives_model.hpp"

#include <QStandardItem>
#include <QFileInfo>
#include <QDir>

class ArchiveItem : public QStandardItem
{
public:
	ArchiveItem(const QString &path)
		: m_path(path)
	{
		setText(QFileInfo(path).fileName());
		setIcon(QIcon(":/icons/brick.png"));
	}
	
	const QString &path() const
	{
		return m_path;
	}
	
private:
	QString m_path;
};

ArchivesModel::ArchivesModel(QObject *parent)
	: QStandardItemModel(parent)
{
}

ArchivesModel::~ArchivesModel()
{
}

void ArchivesModel::setArchivesRoot(const QString &archivesRoot)
{
	m_archivesRoot = archivesRoot;
	refresh();
}

const QString &ArchivesModel::archivesRoot() const
{
	return m_archivesRoot;
}

QString ArchivesModel::path(const QModelIndex index) const
{
	const ArchiveItem *const item = dynamic_cast<const ArchiveItem *>(itemFromIndex(index));
	if(!item) return QString();
	return item->path();
}

QString ArchivesModel::name(const QModelIndex index) const
{
	return QFileInfo(path(index)).fileName();
}

void ArchivesModel::refresh()
{
	clear();
	const QFileInfo rootInfo(m_archivesRoot);
	if(!rootInfo.isDir()) return;
	
	foreach(const QFileInfo &info, rootInfo.dir().entryInfoList()) {
		appendRow(new ArchiveItem(info.absoluteFilePath()));
	}
}