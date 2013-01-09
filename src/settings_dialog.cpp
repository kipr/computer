#include "settings_dialog.hpp"
#include "ui_settings_dialog.h"

#include <QSettings>
#include <QColor>
#include <QFileDialog>
#include <QDebug>

SettingsDialog::SettingsDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::SettingsDialog)
{
	ui->setupUi(this);
	
	connect(ui->customDisplayNameButton, SIGNAL(toggled(bool)), ui->customDisplayNameEdit, SLOT(setEnabled(bool)));
	
	readSettings();
	saveSettings();
}

SettingsDialog::~SettingsDialog()
{
	delete ui;
}

int SettingsDialog::exec()
{
	readSettings();
	if(!QDialog::exec()) return QDialog::Rejected;
	saveSettings();
	return QDialog::Accepted;
}

void SettingsDialog::on_defaultButton_clicked()
{
	QSettings settings;
	settings.clear();
	readSettings();
}

void SettingsDialog::on_programDirectoryBrowse_clicked()
{
	QString path = ui->programDirectory->text();
	path = QFileDialog::getExistingDirectory(this, tr("Choose Program Directory"), path);
	if(path.isEmpty()) return;
	ui->programDirectory->setText(path);
}

void SettingsDialog::on_workingDirectoryBrowse_clicked()
{
	QString path = ui->workingDirectory->text();
	path = QFileDialog::getExistingDirectory(this, tr("Choose Working Directory"), path);
	if(path.isEmpty()) return;
	ui->workingDirectory->setText(path);
}


void SettingsDialog::readSettings()
{
	QSettings settings;
	
	settings.beginGroup(APPEARANCE);
	ui->consoleColorBox->setColor(settings.value(CONSOLE_COLOR, QColor(255, 255, 255)).value<QColor>());
	ui->textColorBox->setColor(settings.value(TEXT_COLOR, QColor(0, 0, 0)).value<QColor>());
	ui->fontBox->setCurrentFont(settings.value(FONT, QFont("Courier New")).value<QFont>());
	ui->fontSizeBox->setValue(settings.value(FONT_SIZE, 14).toInt());
	ui->scrollback->setValue(settings.value(MAXIMUM_SCROLLBACK, 100000).toInt());
	settings.endGroup();
	
	settings.beginGroup(KISS_CONNECTION);
	settings.beginGroup(DISPLAY_NAME);
	const bool def = settings.value(DEFAULT, true).toBool();
	ui->defaultDisplayNameButton->setChecked(def);
	ui->customDisplayNameButton->setChecked(!def);
	ui->customDisplayNameEdit->setText(settings.value(CUSTOM_NAME, "").toString());
	settings.endGroup();
	ui->timeoutBox->setValue(settings.value(TIMEOUT, 5).toInt());
	ui->disallowRemoteBox->setChecked(settings.value(DISALLOW_REMOTE, false).toBool());
	settings.endGroup();
	
	settings.beginGroup(STORAGE);
	ui->programDirectory->setText(settings.value(PROGRAM_DIRECTORY, QDir::homePath() + "/" + tr("KISS Programs")).toString());
	ui->workingDirectory->setText(settings.value(WORKING_DIRECTORY, QDir::homePath() + "/" + tr("KISS Work Dir")).toString());
	settings.endGroup();
}

void SettingsDialog::saveSettings()
{
	QSettings settings;
	const bool defaultChecked = ui->defaultDisplayNameButton->isChecked();
	
	settings.beginGroup(APPEARANCE);
	settings.setValue(CONSOLE_COLOR, ui->consoleColorBox->getColor());
	settings.setValue(TEXT_COLOR, ui->textColorBox->getColor());
	settings.setValue(FONT, ui->fontBox->currentFont());
	settings.setValue(FONT_SIZE, ui->fontSizeBox->value());
	settings.setValue(MAXIMUM_SCROLLBACK, ui->scrollback->value());
	settings.endGroup();
	
	settings.beginGroup(KISS_CONNECTION);
	settings.beginGroup(DISPLAY_NAME);
	settings.setValue(DEFAULT, defaultChecked);
	settings.setValue(CUSTOM_NAME, ui->customDisplayNameEdit->text());
	settings.endGroup();
	settings.setValue(TIMEOUT, ui->timeoutBox->value());
	settings.setValue(DISALLOW_REMOTE, ui->disallowRemoteBox->isChecked());
	settings.endGroup();
	
	settings.beginGroup(STORAGE);
	settings.setValue(PROGRAM_DIRECTORY, ui->programDirectory->text());
	settings.endGroup();
	
	settings.sync();
}
