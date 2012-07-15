#include "SettingsDialog.h"
#include "ui_SettingsDialog.h"

#include <QSettings>

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

void SettingsDialog::readSettings()
{
	QSettings settings;
	
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
	
	settings.beginGroup(KISS_CONNECTION);
	
	settings.beginGroup(DISPLAY_NAME);
	settings.setValue(DEFAULT, defaultChecked);
	settings.setValue(CUSTOM_NAME, ui->customDisplayNameEdit->text());
	settings.endGroup();
	
	settings.setValue(DISALLOW_REMOTE, ui->disallowRemoteBox->isChecked());
	settings.endGroup();
	
	settings.sync();
}
