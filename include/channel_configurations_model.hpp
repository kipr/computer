#ifndef _CHANNELCONFIGURATIONSMODEL_HPP_
#define _CHANNELCONFIGURATIONSMODEL_HPP_

#include <QFileSystemModel>

class ChannelConfigurationsModel : public QFileSystemModel
{
Q_OBJECT
public:
	ChannelConfigurationsModel(QObject *parent = 0);
	~ChannelConfigurationsModel();
	
	QModelIndex defaultConfiguration() const;
private:
	QFileIconProvider *m_iconProvider;
};

#endif
