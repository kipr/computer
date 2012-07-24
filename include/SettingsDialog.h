#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

#define APPEARANCE "appearance"
#define CONSOLE_COLOR "consolecolor"
#define TEXT_COLOR "textcolor"
#define FONT "font"
#define FONT_SIZE "fontsize"
#define KISS_CONNECTION "kissconnection"
#define DISPLAY_NAME "displayname"
#define DEFAULT "default"
#define CUSTOM_NAME "customname"
#define DISALLOW_REMOTE "disallowremote"
#define TIMEOUT "timeout"

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
	Q_OBJECT
	
public:
	explicit SettingsDialog(QWidget *parent = 0);
	~SettingsDialog();
	
	int exec();
	
private slots:
	void on_defaultButton_clicked();
	
private:
	Ui::SettingsDialog *ui;
	
	void readSettings();
	void saveSettings();
};

#endif // SETTINGSDIALOG_H
