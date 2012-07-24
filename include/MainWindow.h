#ifndef _MAINWINDOW_H_
#define _MAINWINDOW_H_

#include <QMainWindow>
#include <QPrinter>
#include <QTime>
#include <QTimer>

#include <easydevice/Server.h>
#include <easydevice/DiscoveryClient.h>
#include <easydevice/ServerDelegate.h>
#include <easydevice/Filesystem.h>
#include <easydevice/PasswordGenerator.h>
#include <kiss-compiler/Compilation.h>

#include "SettingsDialog.h"

namespace Ui
{
	class MainWindow;
}

class QProcess;

class MainWindow : public QMainWindow, public EasyDevice::ServerDelegate
{
Q_OBJECT
public:
	MainWindow(QWidget *parent = 0);
	~MainWindow();
	
	const bool run(const QString& name);
	CompilationPtr compile(const QString& name);
	const bool download(const QString& name, TinyArchive *archive);
	EasyDevice::Filesystem *filesystem();
	
	const bool isAuthenticated(const QHostAddress& address);
	const bool authenticationRequest(const QHostAddress& address);
	const EasyDevice::ServerDelegate::AuthenticateReturn authenticate(const QHostAddress& address, const QByteArray& hash);

private slots:
	void print();
	void saveToFile();
	void about();
	void settings();
	void openWorkingDir();
	
	void timeout();
	void extendTimeout();
	
	void processStarted();
	void processFinished();
	void terminateProcess();
	
private:
	void killProcess();
	void updateSettings();
	QString displayName();
	
	QPrinter m_printer;

	EasyDevice::Server m_server;
	EasyDevice::DiscoveryClient m_discovery;
	
	EasyDevice::Filesystem m_filesystem;
	QMap<QString, QStringList> m_compileResults;
	
	QDir m_workingDirectory;

	SettingsDialog m_settingsDialog;
	
	QProcess *m_process;
	Ui::MainWindow *ui;
	
	QHostAddress m_currentAddress;
	QByteArray m_hash;
	QTimer m_timer;
	QTime m_time;
	EasyDevice::PasswordGenerator m_generator;
};

#endif
