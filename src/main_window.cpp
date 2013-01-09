#include "main_window.hpp"
#include "ui_main_window.h"

#include <pcompiler/pcompiler.hpp>

#include <kovanserial/kovan_serial.hpp>
#include <kovanserial/tcp_server.hpp>

#include "computer.hpp"
#include "quser_info.hpp"
#include "heartbeat.hpp"
#include "server_thread.hpp"

#include <QPrintDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QThreadPool>
#include <QDesktopServices>
#include <QCryptographicHash>
#include <QUrl>
#include <QFileSystemModel>
#include <QClipboard>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent),
	m_process(0),
	ui(new Ui::MainWindow),
	m_timer(this),
	m_heartbeat(new Heartbeat(this)),
	m_server(0)
{
	ui->setupUi(this);
	setWindowTitle(QUserInfo::username() + "'s Computer");
	
	connect(ui->actionPrint, SIGNAL(activated()), this, SLOT(print()));
	connect(ui->actionSave, SIGNAL(activated()), this, SLOT(saveToFile()));
	connect(ui->actionCopy, SIGNAL(activated()), this, SLOT(copy()));
	connect(ui->actionStop, SIGNAL(activated()), this, SLOT(terminateProcess()));
	connect(ui->actionAbout, SIGNAL(activated()), this, SLOT(about()));
	connect(ui->actionSettings, SIGNAL(activated()), this, SLOT(settings()));
	connect(ui->actionOpenWorkingDirectory, SIGNAL(activated()), this, SLOT(openWorkingDir()));
	connect(ui->console, SIGNAL(abortRequested()), this, SLOT(terminateProcess()));
	
	TcpServer *serial = new TcpServer;
	serial->bind(KOVAN_SERIAL_PORT);
	serial->listen(2);
	m_server = new ServerThread(serial);
	connect(m_server, SIGNAL(run(QString)), SLOT(run(QString)));
	m_server->setAutoDelete(true);
	QThreadPool::globalInstance()->start(m_server);
	
	updateSettings();
	
	updateAdvert();
	
	connect(&m_timer, SIGNAL(timeout()), this, SLOT(timeout()));
}

MainWindow::~MainWindow()
{
	killProcess();
	m_server->stop();
	delete ui;
}


void MainWindow::print()
{
	QPrinter printer;
	QPrintDialog *printDialog = new QPrintDialog(&printer, this);
	printDialog->setWindowTitle(tr("Print"));
	if (printDialog->exec() != QDialog::Accepted)
		return;
	ui->console->print(&printer);
}

void MainWindow::saveToFile()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save"), QString(), tr("Text files (*.txt);;All files (*)"));
	if(fileName.isEmpty())
		return;
	
	QFile file(fileName);
	if(!file.open(QFile::WriteOnly | QFile::Text)) {
		QMessageBox::warning(this, tr("Error"), tr("Cannot write to file %1:\n%2.").arg(fileName).arg(file.errorString()));
		return;
	}

	QTextStream out(&file);
	out << ui->console->toPlainText();
}

void MainWindow::copy()
{
	QClipboard *clipboard = QApplication::clipboard();
	QString selection = ui->console->textCursor().selectedText();
	if(selection.isEmpty()) {
		clipboard->setText(ui->console->toPlainText());
		return;
	}
	clipboard->setText(selection);
}

void MainWindow::about()
{
	QString aboutMessage;
	aboutMessage += "Copyright (C) 2012 KISS Institute for Practical Robotics\n\n";
	aboutMessage += "Developed by Braden McDorman and Nafis Zaman\n\n";
	aboutMessage += "Version " + QString::number(COMPUTER_VERSION_MAJOR) + "." + QString::number(COMPUTER_VERSION_MINOR);
	QMessageBox::about(this, "About Computer", aboutMessage);
}

void MainWindow::settings()
{
	if(m_settingsDialog.exec()) updateSettings();
}

void MainWindow::openWorkingDir()
{
	QString path = QDir::toNativeSeparators(m_workingDirectory.absolutePath());
	QDesktopServices::openUrl(QUrl("file:///" + path));
}

void MainWindow::run(const QString &executable)
{
	killProcess();
	m_process = new QProcess();
	QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
	// TODO: This will only work on OS X
#ifdef Q_OS_MAC
	env.insert("DYLD_LIBRARY_PATH", QDir::currentPath() + "/prefix/usr/lib:" + env.value("DYLD_LIBRARY_PATH"));
	env.insert("DYLD_LIBRARY_PATH", QDir::currentPath() + "/prefix/usr:" + env.value("DYLD_LIBRARY_PATH"));
#endif
	m_process->setProcessEnvironment(env);
	m_process->setWorkingDirectory(m_workingDirectory.path());
	ui->console->setProcess(m_process);
	connect(m_process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(processFinished()));
	processStarted();
	m_process->start(executable, QStringList());
	raise();
	extendTimeout();
}

void MainWindow::timeout()
{
	ui->statusbar->showMessage("A command has not been sent in a while. Relocking.", 0);
}

void MainWindow::extendTimeout()
{
	QSettings settings;
	settings.beginGroup(KISS_CONNECTION);
	m_timer.start(settings.value(TIMEOUT).toInt() * 60000);
}

void MainWindow::processStarted()
{
	ui->actionStop->setEnabled(true);
	m_time.restart();
	ui->console->append(tr("Started at %1\n\n").arg(m_time.toString()));
}

void MainWindow::processFinished()
{
	ui->actionStop->setEnabled(false);
	const int msecs = m_time.elapsed();
	m_time.restart();
	ui->console->append(tr("\nFinished at %1 in %2 seconds").arg(m_time.toString()).arg(msecs / 1000.0));
}

void MainWindow::terminateProcess()
{
	if(!m_process) return;
	m_process->terminate();
	if(!m_process->waitForFinished(2000)) m_process->kill();
	ui->console->setProcess(0);
	delete m_process;
	m_process = 0;
}

void MainWindow::killProcess()
{
	if(!m_process) return;
	m_process->kill();
	m_process->waitForFinished();
	ui->console->setProcess(0);
	delete m_process;
	m_process = 0;
}

void MainWindow::updateSettings()
{
	QSettings settings;
	settings.beginGroup(APPEARANCE);
	QColor consoleColor = settings.value(CONSOLE_COLOR).value<QColor>();
	QColor textColor = settings.value(TEXT_COLOR).value<QColor>();
	QFont font = settings.value(FONT).value<QFont>();
	qreal fontSize = settings.value(FONT_SIZE).toInt();
	ui->console->document()->setMaximumBlockCount(settings.value(MAXIMUM_SCROLLBACK, 100000).toInt());
	settings.endGroup();

	updateAdvert();
	
	QPalette pal = ui->console->palette();
	pal.setColor(QPalette::Base, consoleColor);
	ui->console->setPalette(pal);
	
	QString contents = ui->console->toPlainText();
	ui->console->clear();
	ui->console->setTextColor(textColor);
	ui->console->setCurrentFont(font);
	ui->console->setFontPointSize(fontSize);
	ui->console->setPlainText(contents);
	
	settings.beginGroup(STORAGE);
	QString workPath = settings.value(WORKING_DIRECTORY, QDir::homePath() + "/" + tr("KISS Work Dir")).toString();
	
	m_workingDirectory = QDir(workPath);
	if(!m_workingDirectory.exists()) QDir().mkpath(workPath);
	
	settings.endGroup();
}

void MainWindow::updateAdvert()
{
	QString version = (QString::number(COMPUTER_VERSION_MAJOR) + "." + QString::number(COMPUTER_VERSION_MINOR));
	#if defined(Q_OS_WIN)
	version += " for Windows";
	#elif defined(Q_OS_MAC)
	version += " for Mac OS X";
	#else
	version += " for *nix";
	#endif
	
	Advert ad(tr("N/A").toAscii(),
		version.toAscii(),
		tr("computer").toAscii(),
		displayName().toAscii());
	m_heartbeat->setAdvert(ad);
}

QString MainWindow::displayName()
{
	QString ret;
	
	QSettings settings;
	settings.beginGroup(KISS_CONNECTION);
	settings.beginGroup(DISPLAY_NAME);
	if(settings.value(DEFAULT).toBool()) ret = QUserInfo::username() + "'s Computer";
	else ret = settings.value(CUSTOM_NAME).toString();
	settings.endGroup();
	settings.endGroup();
	
	return ret;
}


QString MainWindow::cachePath(const QString& name) const
{
	QString ret = programSavePath(name) + ".cache";
	QDir().mkpath(ret);
	return ret;
}

QString MainWindow::programSavePath(const QString& name) const
{
	return QString();
}
