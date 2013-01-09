#ifndef _MAIN_WINDOW_HPP_
#define _MAIN_WINDOW_HPP_

#include "settings_dialog.hpp"

#include <QMainWindow>
#include <QPrinter>
#include <QTime>
#include <QItemSelection>
#include <QTimer>
#include <QDir>

namespace Ui
{
	class MainWindow;
}

class QProcess;
class QFileSystemModel;
class Heartbeat;
class ServerThread;

class MainWindow : public QMainWindow
{
Q_OBJECT
public:
	MainWindow(QWidget *parent = 0);
	~MainWindow();

private slots:
	void print();
	void saveToFile();
	void copy();
	void about();
	void settings();
	void openWorkingDir();
	
	void run(const QString &executable);
	
	void timeout();
	void extendTimeout();
	
	void processStarted();
	void processFinished();
	void terminateProcess();
	
private:
	void killProcess();
	void updateSettings();
	void updateAdvert();
	
	QString displayName();
	QString tempPath() const;
	QString cachePath(const QString& name) const;
	QString programSavePath(const QString& name) const;

	QMap<QString, QString> m_compileResults;
	
	QDir m_workingDirectory;

	SettingsDialog m_settingsDialog;
	
	QProcess *m_process;
	Ui::MainWindow *ui;
	
	QByteArray m_hash;
	QTimer m_timer;
	QTime m_time;
	
	QFileSystemModel *m_filesystemModel;
	
	Heartbeat *m_heartbeat;
	ServerThread *m_server;
};

#endif
