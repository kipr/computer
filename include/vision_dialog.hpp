#ifndef _VISION_DIALOG_HPP_
#define _VISION_DIALOG_HPP_

#include <QDialog>
#include <QTimer>
#include <QModelIndex>
#include <QItemSelection>

#include <kovan/config.hpp>

namespace Ui
{
	class VisionDialog;
}

namespace Camera
{
	class Device;
}

class ChannelConfigurationsModel;
class CameraConfigModel;

class VisionDialog : public QDialog
{
Q_OBJECT
public:
	VisionDialog(QWidget *parent);
	~VisionDialog();
	
	virtual int exec();
	bool isDefaultPath(const QModelIndex &index) const;
	
private Q_SLOTS:
	void on_addConfig_clicked();
	void on_removeConfig_clicked();
	void on_renameConfig_clicked();
	void on_defaultConfig_clicked();
	void on_addChannel_clicked();
	void on_removeChannel_clicked();
	void on_up_clicked();
	void on_down_clicked();
	
	void updateCamera();
	void manualEntry(const QString &number);
	void visualChanged();
	void imagePressed(const int x, const int y);
	
	void currentConfigChanged(const QModelIndex &current, const QModelIndex &prev);
	void updateOptions(const QItemSelection &current, const QItemSelection &prev);
	
private:
	void refreshHsv();
	void blockChildSignals(const bool block);
	
	ChannelConfigurationsModel *m_configsModel;
	CameraConfigModel *m_configModel;
	
	Ui::VisionDialog *ui;
	QTimer m_cameraTimer;
	
	Camera::Device *m_device;
	Config m_hsvConfig;
	QString m_defaultPath;
	
	bool m_ignoreNextConfigSave;
};

#endif
