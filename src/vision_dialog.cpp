#include "vision_dialog.hpp"
#include "ui_vision_dialog.h"

#include "channel_configurations_model.hpp"
#include "camera_config_model.hpp"
#include "input_dialog.hpp"

#include <kovan/camera.hpp>

#include <QItemSelection>
#include <QFileInfo>
#include <QItemDelegate>
#include <QPainter>
#include <QDir>
#include <QDebug>

class ConfigItemDelegate : public QItemDelegate
{
public:
	ConfigItemDelegate(VisionDialog *parent = 0)
		: QItemDelegate(parent),
		m_star(QIcon(":/icons/star.png").pixmap(16, 16))
	{
	}
	
	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
	{
		QItemDelegate::paint(painter, option, index);
		VisionDialog *w = qobject_cast<VisionDialog *>(parent());
		if(!w->isDefaultPath(index)) return;
		const QPoint right = option.rect.topRight();
		painter->drawPixmap(right.x() - 24, right.y() + option.rect.height() / 2 - 8,
			16, 16, m_star);
	}
	
	QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
	{
		return QSize(0, 22);
	}
	
private:
	QPixmap m_star;
};

class ChannelItemDelegate : public QItemDelegate
{
public:
	ChannelItemDelegate(CameraConfigModel *model, QObject *parent = 0)
		: QItemDelegate(parent),
		m_model(model),
		m_hsv(QIcon(":/icons/color_wheel.png").pixmap(16, 16)),
		m_qr(QIcon(":/icons/qr.png").pixmap(16, 16))
	{
	}
	
	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
	{
		QItemDelegate::paint(painter, option, index);

		const QString &type = m_model->channelType(index);

		QPixmap pixmap;
		if(type == CAMERA_CHANNEL_TYPE_HSV_KEY) pixmap = m_hsv;
		else if(type == CAMERA_CHANNEL_TYPE_QR_KEY) pixmap = m_qr;

		const QPoint right = option.rect.topRight();
		painter->drawPixmap(right.x() - 24, right.y() + option.rect.height() / 2 - 8,
			16, 16, pixmap);
	}
	
private:
	CameraConfigModel *m_model;
	QPixmap m_hsv;
	QPixmap m_qr;
};

VisionDialog::VisionDialog(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::VisionDialog)
	, m_configsModel(new ChannelConfigurationsModel(this))
	, m_configModel(new CameraConfigModel(this))
	, m_device(new Camera::Device(new Camera::UsbInputProvider))
	, m_ignoreNextConfigSave(false)
{
	ui->setupUi(this);
	
	ui->configs->setModel(m_configsModel);
	ui->configs->setRootIndex(m_configsModel->index(m_configsModel->rootPath()));
	ui->configs->setItemDelegate(new ConfigItemDelegate(this));
	
	ui->channels->setModel(m_configModel);
	ui->channels->setItemDelegate(new ChannelItemDelegate(m_configModel, this));
	
	connect(&m_cameraTimer, SIGNAL(timeout()), SLOT(updateCamera()));
	m_cameraTimer.start(100);
	
	Config deviceConfig;
	deviceConfig.beginGroup(CAMERA_GROUP);
	deviceConfig.setValue(CAMERA_NUM_CHANNELS_KEY, 1);
	deviceConfig.beginGroup((QString(CAMERA_CHANNEL_GROUP_PREFIX) + "0").toStdString());
	deviceConfig.setValue(CAMERA_CHANNEL_TYPE_KEY, CAMERA_CHANNEL_TYPE_HSV_KEY);
	deviceConfig.clearGroup();
	m_device->setConfig(deviceConfig);
	
	connect(ui->_th, SIGNAL(textChanged(QString)), SLOT(manualEntry(QString)));
	connect(ui->_ts, SIGNAL(textChanged(QString)), SLOT(manualEntry(QString)));
	connect(ui->_tv, SIGNAL(textChanged(QString)), SLOT(manualEntry(QString)));
	connect(ui->_bh, SIGNAL(textChanged(QString)), SLOT(manualEntry(QString)));
	connect(ui->_bs, SIGNAL(textChanged(QString)), SLOT(manualEntry(QString)));
	connect(ui->_bv, SIGNAL(textChanged(QString)), SLOT(manualEntry(QString)));
	
	connect(ui->visual, SIGNAL(minChanged(QColor)), SLOT(visualChanged()));
	connect(ui->visual, SIGNAL(maxChanged(QColor)), SLOT(visualChanged()));
	
	connect(ui->cv, SIGNAL(pressed(int, int)), SLOT(imagePressed(int, int)));
	
	connect(ui->channels->selectionModel(),
		SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
		SLOT(updateOptions(QItemSelection, QItemSelection)));
	
	connect(ui->configs->selectionModel(),
		SIGNAL(currentChanged(QModelIndex, QModelIndex)),
		SLOT(currentConfigChanged(QModelIndex, QModelIndex)));
		
	m_defaultPath = m_configsModel->filePath(m_configsModel->defaultConfiguration());
}

VisionDialog::~VisionDialog()
{
	delete m_device;
}

void VisionDialog::updateCamera()
{
	if(!m_device->update()) {
		qWarning() << "Lost camera";
		ui->cv->setInvalid(true);
		return;
	}

	cv::Mat image = m_device->rawImage();
	
	if(!ui->hsv->isEnabled()) {
		ui->cv->updateImage(image);
		return;
	}

	const Camera::ObjectVector *objs = m_device->channels()[0]->objects();
	if(!objs) {
		qWarning() << "Objects invalid";
		ui->cv->setInvalid(true);
		return;
	}
	
	Camera::ObjectVector::const_iterator it = objs->begin();
	
	if(image.empty()) {
		qDebug() << "Empty???";
	}
	
	for(; it != objs->end(); ++it) {
		const Camera::Object &obj = *it;
		cv::rectangle(image, cv::Rect(obj.boundingBox().x(), obj.boundingBox().y(),
			obj.boundingBox().width(), obj.boundingBox().height()),
			cv::Scalar(255, 0, 0), 2);
	}
	
	ui->cv->updateImage(image);
}

int VisionDialog::exec()
{
	if(!m_device->open()) return QDialog::Rejected;
	
	int ret = QDialog::exec();
	m_device->close();
	
	QItemSelection selection = ui->configs->selectionModel()->selection();
	if(selection.indexes().size())
		currentConfigChanged(QModelIndex(), selection.indexes()[0]);
	
	return ret;
}

void VisionDialog::on_addChannel_clicked()
{
	m_configModel->addChannel(CAMERA_CHANNEL_TYPE_HSV_KEY);
}

void VisionDialog::on_removeChannel_clicked()
{
	const QModelIndexList &indexes = ui->channels->selectionModel()
		->selection().indexes();
	if(indexes.size() != 1) return;
	m_configModel->removeChannel(indexes[0].row());
}

void VisionDialog::on_up_clicked()
{
	QItemSelectionModel *selModel = ui->channels->selectionModel();
	const QModelIndexList &indexes = selModel->selection().indexes();
	if(indexes.size() != 1) return;
	const int i = indexes[0].row();
	if(i - 1 < 0) return;
	m_configModel->swapChannels(i, i - 1);
	selModel->clearSelection();
	selModel->select(m_configModel->item(i - 1)->index(),
		QItemSelectionModel::Select);
}

void VisionDialog::on_down_clicked()
{
	QItemSelectionModel *selModel = ui->channels->selectionModel();
	const QModelIndexList &indexes = selModel->selection().indexes();
	if(indexes.size() != 1) return;
	const int i = indexes[0].row();
	if(i + 1 >= m_configModel->rowCount()) return;
	m_configModel->swapChannels(i, i + 1);
	selModel->clearSelection();
	selModel->select(m_configModel->item(i + 1)->index(),
		QItemSelectionModel::Select);
}

void VisionDialog::updateOptions(const QItemSelection &current, const QItemSelection &prev)
{
	if(prev.indexes().size()) {
		QModelIndex p = prev.indexes()[0];
		Config c = m_configModel->config();
		c.beginGroup(CAMERA_GROUP);
		c.beginGroup(CAMERA_CHANNEL_GROUP_PREFIX + QString::number(p.row()).toStdString());
		c.addValues(m_hsvConfig);
		m_configModel->setConfig(c);
		ui->channels->selectionModel()->select(current, QItemSelectionModel::Select);
	}
	
	const QModelIndexList &indexes = current.indexes();
	const bool enable = ui->channels->isEnabled();
	const bool sel = indexes.size() == 1 && enable;
	ui->addChannel->setEnabled(enable);
	ui->removeChannel->setEnabled(sel);
	ui->up->setEnabled(sel && indexes[0].row() > 0);
	ui->down->setEnabled(sel && indexes[0].row() + 1 < m_configModel->rowCount());
	ui->hsv->setEnabled(sel);
	
	if(indexes.size()) {
		Config c = m_configModel->config();
		c.beginGroup(CAMERA_GROUP);
		c.beginGroup((CAMERA_CHANNEL_GROUP_PREFIX + QString::number(indexes[0].row()))
			.toStdString());
		m_hsvConfig = c.values();
	}
	refreshHsv();
}

void VisionDialog::refreshHsv()
{
	if(!ui->hsv->isEnabled()) {
		ui->_th->setText(QString());
		ui->_ts->setText(QString());
		ui->_tv->setText(QString());
		ui->_bh->setText(QString());
		ui->_bs->setText(QString());
		ui->_bv->setText(QString());
		return;
	}
	
	int th = m_hsvConfig.intValue("th") * 2;
	int ts = m_hsvConfig.intValue("ts");
	int tv = m_hsvConfig.intValue("tv");
	int bh = m_hsvConfig.intValue("bh") * 2;
	int bs = m_hsvConfig.intValue("bs");
	int bv = m_hsvConfig.intValue("bv");

	if(th == bh) {
		th += 5;
		th %= 360;
		bh -= 5;
		if(bh < 0) bh += 360;
	}
	
	if(ts == bs) {
		ts += 5;
		ts = ts > 255 ? 255 : ts;
		bs -= 5;
		bs = bs < 0 ? 0 : bs;
	}
	
	if(tv == bv) {
		tv += 5;
		tv = tv > 255 ? 255 : tv;
		bv -= 5;
		bv = bv < 0 ? 0 : bv;
	}
	
	// Visual
	ui->visual->setMax(QColor::fromHsv(th, ts, tv));
	ui->visual->setMin(QColor::fromHsv(bh, bs, bv));
	
	// Manual
	ui->_th->setText(QString::number(th));
	ui->_ts->setText(QString::number(ts));
	ui->_tv->setText(QString::number(tv));
	ui->_bh->setText(QString::number(bh));
	ui->_bs->setText(QString::number(bs));
	ui->_bv->setText(QString::number(bv));
	
	m_device->channels()[0]->setConfig(m_hsvConfig);
}

void VisionDialog::manualEntry(const QString &number)
{
	QObject *from = sender();
	if(!from || !ui->hsv->isEnabled()) return;
	
	int num = number.toInt();
	if(num < 0) num = 0;
	
	if(from == ui->_th) {
		m_hsvConfig.setValue("th", qMin(num, 359) >> 1);
	} else if(from == ui->_ts) {
		m_hsvConfig.setValue("ts", qMin(num, 255));
	} else if(from == ui->_tv) {
		m_hsvConfig.setValue("tv", qMin(num, 255));
	} else if(from == ui->_bh) {
		m_hsvConfig.setValue("bh", qMin(num, 359) >> 1);
	} else if(from == ui->_bs) {
		m_hsvConfig.setValue("bs", qMin(num, 255));
	} else if(from == ui->_bv) {
		m_hsvConfig.setValue("bv", qMin(num, 255));
	}
	
	blockChildSignals(true);
	refreshHsv();
	blockChildSignals(false);
}

void VisionDialog::visualChanged()
{
	if(!ui->hsv->isEnabled()) return;
	
	const QColor &max = ui->visual->max();
	const QColor &min = ui->visual->min();
	
	m_hsvConfig.setValue("th", max.hue() / 2);
	m_hsvConfig.setValue("ts", max.saturation());
	m_hsvConfig.setValue("tv", max.value());
	m_hsvConfig.setValue("bh", min.hue() / 2);
	m_hsvConfig.setValue("bs", min.saturation());
	m_hsvConfig.setValue("bv", min.value());
	
	blockChildSignals(true);
	refreshHsv();
	blockChildSignals(false);
}

void VisionDialog::imagePressed(const int x, const int y)
{
	if(!ui->hsv->isEnabled()) return;
	
	cv::Mat image = m_device->rawImage();
	cv::Vec3b data = image.at<cv::Vec3b>(y, x);
	const QColor c(data[2], data[1], data[0]);
	
	int th = (c.hue() / 2 + 5) % 180;
	int ts = c.saturation() + 40;
	int tv = c.value() + 40;
	int bh = c.hue() / 2 - 5;
	int bs = c.saturation() - 40;
	int bv = c.value() - 40;
	
	if(ts > 255) ts = 255;
	if(tv > 255) tv = 255;
	
	if(bh < 0) bh += 180;
	if(bs < 0) bs = 0;
	if(bv < 0) bv = 0;
	
	qDebug() << "touch bh: " << bh;
	
	m_hsvConfig.setValue("th", th);
	m_hsvConfig.setValue("ts", ts);
	m_hsvConfig.setValue("tv", tv);
	m_hsvConfig.setValue("bh", bh);
	m_hsvConfig.setValue("bs", bs);
	m_hsvConfig.setValue("bv", bv);
	
	blockChildSignals(true);
	refreshHsv();
	blockChildSignals(false);
}

void VisionDialog::blockChildSignals(const bool block)
{
	ui->_th->blockSignals(block);
	ui->_ts->blockSignals(block);
	ui->_tv->blockSignals(block);
	ui->_bh->blockSignals(block);
	ui->_bs->blockSignals(block);
	ui->_bv->blockSignals(block);
	ui->visual->blockSignals(block);
}

bool VisionDialog::isDefaultPath(const QModelIndex &index) const
{
	return m_configsModel->filePath(index) == m_defaultPath;
}

void VisionDialog::on_renameConfig_clicked()
{
	QItemSelection selection = ui->configs->selectionModel()->selection();
	if(selection.indexes().size() != 1) return;
	QModelIndex index = selection.indexes()[0];
	
	QFileInfo file = m_configsModel->fileInfo(index);
	
	InputDialog input(this);
	input.setKey(tr("Configuration Name"));
	input.setValue(file.baseName());
	input.setEmptyAllowed(false);
	if(input.exec() != QDialog::Accepted) return;
	
	// Forgive me, programming gods
	m_ignoreNextConfigSave = true;
	if(!QFile::rename(file.filePath(),
		file.path() + "/" + input.value() + "." + file.completeSuffix())) {
		qWarning() << "Failed to change name";
	}
}

void VisionDialog::on_defaultConfig_clicked()
{
	QItemSelection selection = ui->configs->selectionModel()->selection();
	if(selection.indexes().size() != 1) return;
	
	QModelIndex index = selection.indexes()[0];
	m_defaultPath = m_configsModel->filePath(index);
	ui->defaultConfig->setEnabled(false);
	Camera::ConfigPath::setDefaultConfigPath(m_configsModel->fileInfo(index)
		.baseName().toStdString());
	ui->configs->repaint();
}

void VisionDialog::on_addConfig_clicked()
{
	InputDialog input(this);
	input.setKey(tr("Configuration Name"));
	input.setEmptyAllowed(false);
	if(input.exec() != QDialog::Accepted) return;
	
	Config blank;
	std::string savePath = Camera::ConfigPath::path(input.value().toStdString());
	QString qSavePath = QString::fromStdString(savePath);
	if(!blank.save(savePath)) {
		qWarning() << "Error saving" << qSavePath;
		return;
	}
	
	// Select it and set as default if it's the first one
	QDir saves = QDir(QFileInfo(qSavePath).path(), "*." + QString::fromStdString(
		Camera::ConfigPath::extension()));
	if(saves.entryList(QDir::Files).size() == 1) {
		QModelIndex index = m_configsModel->index(qSavePath);
		ui->configs->selectionModel()->select(index, QItemSelectionModel::Select);
		on_defaultConfig_clicked();
		currentConfigChanged(index, QModelIndex());
	}
}

void VisionDialog::on_removeConfig_clicked()
{
	QItemSelection selection = ui->configs->selectionModel()->selection();
	if(selection.indexes().size() != 1) return;
	// Forgive me, programming gods
	m_ignoreNextConfigSave = true;
	QFile::remove(m_configsModel->filePath(selection.indexes()[0]));
}

void VisionDialog::currentConfigChanged(const QModelIndex &current, const QModelIndex &prev)
{
	if(prev.isValid() && !m_ignoreNextConfigSave) {
		const Config c = m_configModel->config();
		c.save(Camera::ConfigPath::path(m_configsModel->fileInfo(prev)
					.baseName().toStdString()));
		m_ignoreNextConfigSave = false;
	}
	
	const bool enable = current.isValid();
	ui->renameConfig->setEnabled(enable);
	ui->defaultConfig->setEnabled(enable && !isDefaultPath(current));
	ui->removeConfig->setEnabled(enable);
	ui->channels->setEnabled(enable);
	if(enable) {
		Config *config = Config::load(Camera::ConfigPath::path(m_configsModel->fileInfo(current)
			.baseName().toStdString()));
		if(config) m_configModel->setConfig(*config);
		delete config;
	} else {
		m_configModel->setConfig(Config());
	}
	
	updateOptions(QItemSelection(), ui->channels->selectionModel()->selection());
}
