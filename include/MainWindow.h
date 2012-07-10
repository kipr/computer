#ifndef _MAINWINDOW_H_
#define _MAINWINDOW_H_

#include <QMainWindow>
#include <QPrinter>

#include <easydevice/Server.h>
#include <easydevice/DiscoveryClient.h>
#include <easydevice/ServerDelegate.h>
#include <easydevice/Filesystem.h>
#include <kiss-compiler/Compilation.h>

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

public slots:
	void print();
	void saveToFile();
	
private:
	void killProcess();
	
	QPrinter m_printer;

	EasyDevice::Server m_server;
	EasyDevice::DiscoveryClient m_discovery;
	
	EasyDevice::Filesystem m_filesystem;
	QMap<QString, QStringList> m_compileResults;
	
	QDir m_workingDirectory;
	
	QProcess *m_process;
	Ui::MainWindow *ui;
};

#endif
