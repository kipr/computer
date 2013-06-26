#include "channel_configurations_model.hpp"

#include <QDir>
#include <QFileIconProvider>

#include <kovan/camera.hpp>

#include <QDebug>

class FileIconProvider : public QFileIconProvider
{
public:
	FileIconProvider()
		: m_icon(QIcon(":/icons/photos.png"))
	{
	}
	
	virtual QIcon icon(const QFileInfo &info) const
	{
		return m_icon;
	}
	
private:
	QIcon m_icon;
};

ChannelConfigurationsModel::ChannelConfigurationsModel(QObject *parent)
	: QFileSystemModel(parent),
	m_iconProvider(new FileIconProvider)
{
	setIconProvider(m_iconProvider);
	const QString configPath = QString::fromStdString(Camera::ConfigPath::path());
	qDebug() << configPath;
	QDir().mkpath(configPath);
	setNameFilters(QStringList() << ("*." + QString::fromStdString(Camera::ConfigPath::extension())));
	setNameFilterDisables(false);
	setFilter(QDir::NoDot | QDir::NoDotDot | QDir::Files);
	setRootPath(configPath);
}

ChannelConfigurationsModel::~ChannelConfigurationsModel()
{
	delete m_iconProvider;
}

QModelIndex ChannelConfigurationsModel::defaultConfiguration() const
{
	return index(QString::fromStdString(Camera::ConfigPath::defaultConfigPath()));
}