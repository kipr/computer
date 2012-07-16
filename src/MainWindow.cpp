#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <easydevice/DiscoveryConstants.h>

#include <kiss-compiler/ArchiveWriter.h>
#include <kiss-compiler/Temporary.h>
#include <kiss-compiler/CompilerManager.h>

#include "Computer.h"
#include "QUserInfo.h"
#include <QPrintDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QDebug>

using namespace EasyDevice;

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent),
	m_server(this),
	m_discovery(this),
	m_process(0),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	
	ui->menuFile->addAction(ui->actionPrint);
	ui->menuFile->addAction(ui->actionSave);
	
	DeviceInfo deviceInfo;
	deviceInfo.setDeviceType("computer");
	deviceInfo.setDisplayName(displayName());
	deviceInfo.setSerialNumber("N/A");

	QString version = (QString::number(COMPUTER_VERSION_MAJOR) + "." + QString::number(COMPUTER_VERSION_MINOR));
	#if defined(Q_OS_WIN)
	version += " for Windows";
	#elif defined(Q_OS_MAC)
	version += " for Mac OS X";
	#else
	version += " for *nix";
	#endif
	deviceInfo.setVersion(version);
	
	m_discovery.setDeviceInfo(deviceInfo);
	
	bool success = true;
	if(!m_discovery.setup()) {
		qDebug() << "Failed to setup listener";
		success &= false;
	}
	if(!m_server.listen(QHostAddress::Any, 8075)) {
		qDebug() << "Failed to listen";
		success &= false;
	}
	
	if(success) ui->statusbar->showMessage(QString("Listening for connections on port %1").arg(8075), 0);
	else ui->statusbar->showMessage("Error listening for incoming connections", 0);

	connect(ui->actionPrint, SIGNAL(activated()), this, SLOT(print()));
	connect(ui->actionSave, SIGNAL(activated()), this, SLOT(saveToFile()));
	connect(ui->actionAbout, SIGNAL(activated()), this, SLOT(about()));
	connect(ui->actionSettings, SIGNAL(activated()), this, SLOT(settings()));
	
	updateSettings();
}

MainWindow::~MainWindow()
{
	killProcess();
	delete ui;
}

const bool MainWindow::run(const QString& name)
{
	if(!m_filesystem.program(name)) return false;

	qDebug() << name << "has the following results avail for running:" << m_compileResults.value(name);
	if(!m_compileResults.value(name).size()) {
		compile(name);
	}
	if(!m_compileResults.value(name).size()) {
		qCritical() << "Cannot run" << name << ". No results.";
		return false;
	}

	killProcess();
	m_process = new QProcess();
	m_process->setWorkingDirectory(m_workingDirectory.path());
	ui->console->setProcess(m_process);
	m_process->start(m_compileResults.value(name)[0], QStringList());

	return true;
}

CompilationPtr MainWindow::compile(const QString& name)
{
	TinyArchive *archive = m_filesystem.program(name);
	if(!archive) return CompilationPtr();
	
	ArchiveWriter writer(archive, Temporary::subdir(name));
	QMap<QString, QString> settings;
	QByteArray rawSettings = QTinyNode::data(archive->lookup("settings:"));
	QDataStream stream(rawSettings);
	stream >> settings;
	
	CompilationPtr compilation(new Compilation(CompilerManager::ref().compilers(), name, writer.files(), settings, "kovan"));
	bool success = compilation->start();
	qDebug() << "Results:" << compilation->compileResults();
	
	qDebug() << (success ? "Compile Succeeded" : "Compile Failed");
	
	if(success) m_compileResults[name] = compilation->compileResults();
	else m_compileResults.remove(name);
	
	return compilation;
}

const bool MainWindow::download(const QString& name, TinyArchive *archive)
{
	m_filesystem.setProgram(name, archive);
	m_compileResults.remove(name);
	return true;
}

Filesystem *MainWindow::filesystem()
{
	return &m_filesystem;
}

void MainWindow::print()
{
	QPrintDialog *printDialog = new QPrintDialog(&m_printer, this);
	printDialog->setWindowTitle(tr("Print"));
	if (printDialog->exec() != QDialog::Accepted)
		return;
	ui->console->print(&m_printer);
}

void MainWindow::saveToFile()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save"), QString(), tr("Text files (*.txt);;All files (*)"));
	if(fileName.isEmpty())
		return;
	
	QFile file(fileName);
	if(!file.open(QFile::WriteOnly | QFile::Text))
	{
		QMessageBox::warning(this, tr("Error"), tr("Cannot write to file %1:\n%2.").arg(fileName).arg(file.errorString()));
		return;
	}

	QTextStream out(&file);
	out << ui->console->toPlainText();
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
	DeviceInfo deviceInfo = m_discovery.deviceInfo();
	deviceInfo.setDisplayName(displayName());
	m_discovery.setDeviceInfo(deviceInfo);
	
	QSettings settings;
	settings.beginGroup(APPEARANCE);
	QColor consoleColor = settings.value(CONSOLE_COLOR, QColor(255, 255, 255)).value<QColor>();
	QColor textColor = settings.value(TEXT_COLOR, QColor(255, 255, 255)).value<QColor>();
	settings.endGroup();
	
	QPalette pal = ui->console->palette();
	pal.setColor(QPalette::Base, consoleColor);
	ui->console->setPalette(pal);
	
	QString contents = ui->console->toPlainText();
	ui->console->clear();
	ui->console->setTextColor(textColor);
	ui->console->setPlainText(contents);
}

QString MainWindow::displayName()
{
	QString ret;
	
	QSettings settings;
	settings.beginGroup(KISS_CONNECTION);
	settings.beginGroup(DISPLAY_NAME);
	if(settings.value(DEFAULT).toBool())
		ret = QUserInfo::username();
	else
		ret = settings.value(CUSTOM_NAME).toString();
	settings.endGroup();
	settings.endGroup();
	
	return ret;
}
