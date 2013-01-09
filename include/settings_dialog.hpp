#ifndef _SETTINGS_DIALOG_HPP_
#define _SETTINGS_DIALOG_HPP_

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
#define STORAGE "storage"
#define PROGRAM_DIRECTORY "programdir"
#define WORKING_DIRECTORY "workdir"
#define CONSOLE
#define MAXIMUM_SCROLLBACK "maxscrollback"

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
	void on_programDirectoryBrowse_clicked();
	void on_workingDirectoryBrowse_clicked();
	
private:
	Ui::SettingsDialog *ui;
	
	void readSettings();
	void saveSettings();
};

#endif // SETTINGSDIALOG_H