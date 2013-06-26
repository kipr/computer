#ifndef _ARCHIVES_MODEL_HPP_
#define _ARCHIVES_MODEL_HPP_

#include <QStandardItemModel>
#include <QString>

class ArchivesModel : public QStandardItemModel
{
Q_OBJECT
public:
	ArchivesModel(QObject *parent);
	~ArchivesModel();
	
	Q_PROPERTY(QString archivesRoot READ archivesRoot WRITE setArchivesRoot);
	void setArchivesRoot(const QString &archivesRoot);
	const QString &archivesRoot() const;
	
	QString path(const QModelIndex index) const;
	QString name(const QModelIndex index) const;
	
private:
	void refresh();
	
	QString m_archivesRoot;
};

#endif
