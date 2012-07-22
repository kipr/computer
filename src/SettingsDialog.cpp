#include "SettingsDialog.h"
#include "ui_SettingsDialog.h"

#include <QSettings>
#include <QColor>
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
	if(!QDialog::exec())
		return QDialog::Rejected;
	saveSettings();
	return QDialog::Accepted;
}

void SettingsDialog::on_defaultButton_clicked()
{
	QSettings settings;
	settings.clear();
	readSettings();
}

void SettingsDialog::readSettings()
{
	QSettings settings;
	
	settings.beginGroup(APPEARANCE);
	ui->consoleColorBox->setColor(settings.value(CONSOLE_COLOR, QColor(255, 255, 255)).value<QColor>());
	ui->textColorBox->setColor(settings.value(TEXT_COLOR, QColor(0, 0, 0)).value<QColor>());
	ui->fontBox->setCurrentFont(settings.value(FONT, QFont("Courier New")).value<QFont>());
	ui->fontSizeBox->setValue(settings.value(FONT_SIZE, 14).toInt());
	settings.endGroup();
	
	settings.beginGroup(KISS_CONNECTION);
	settings.beginGroup(DISPLAY_NAME);
	if(settings.value(DEFAULT, true).toBool())
		ui->defaultDisplayNameButton->setChecked(true);
	else
		ui->customDisplayNameButton->setChecked(true);
	ui->customDisplayNameEdit->setText(settings.value(CUSTOM_NAME, "").toString());
	settings.endGroup();
	ui->disallowRemoteBox->setChecked(settings.value(DISALLOW_REMOTE, false).toBool());
	settings.endGroup();
}

void SettingsDialog::saveSettings()
{
	QSettings settings;
	bool defaultChecked = ui->defaultDisplayNameButton->isChecked();
	
	settings.beginGroup(APPEARANCE);
	settings.setValue(CONSOLE_COLOR, ui->consoleColorBox->getColor());
	settings.setValue(TEXT_COLOR, ui->textColorBox->getColor());
	settings.setValue(FONT, ui->fontBox->currentFont());
	settings.setValue(FONT_SIZE, ui->fontSizeBox->value());
	settings.endGroup();
	
	settings.beginGroup(KISS_CONNECTION);
	settings.beginGroup(DISPLAY_NAME);
	settings.setValue(DEFAULT, defaultChecked);
	settings.setValue(CUSTOM_NAME, ui->customDisplayNameEdit->text());
	settings.endGroup();
	settings.setValue(DISALLOW_REMOTE, ui->disallowRemoteBox->isChecked());
	settings.endGroup();
	
	settings.sync();
}
