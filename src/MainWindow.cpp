#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <easydevice/DiscoveryConstants.h>

#include <kiss-compiler/ArchiveWriter.h>
#include <kiss-compiler/Temporary.h>
#include <kiss-compiler/CompilerManager.h>

#include "Computer.h"
#include "QUserInfo.h"

using namespace EasyDevice;

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent),
	m_server(this),
	m_discovery(this),
	m_process(0),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	
	DeviceInfo deviceInfo;
	deviceInfo.setDeviceType("computer");
	deviceInfo.setDisplayName(QUserInfo::username() + "'s Computer");
	deviceInfo.setSerialNumber("N/A");

    QString version = (QString::number(COMPUTER_VERSION_MAJOR) + "." + QString::number(COMPUTER_VERSION_MINOR));
    #if defined(Q_OS_WIN)
    version += " (for Windows)";
    #elif defined(Q_OS_MAC)
    version += " (for Mac)";
    #else
    version += " (for Linux)";
    #endif
    deviceInfo.setVersion(version);
	
	m_discovery.setDeviceInfo(deviceInfo);
	
	bool success = true;
	
	if(!m_discovery.setup()) {
		qDebug() << "Failed to setup ohaiyo listener";
		success &= false;
	}
	if(!m_server.listen(QHostAddress::Any, 8075)) {
		qDebug() << "Failed to listen";
		success &= false;
	}
	
	if(success) ui->statusbar->showMessage(QString("Listening for connections on port %1").arg(8075), 0);
	else ui->statusbar->showMessage("Error listening for incoming connections", 0);
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

void MainWindow::killProcess()
{
	if(!m_process) return;
	m_process->kill();
	m_process->waitForFinished();
	ui->console->setProcess(0);
	delete m_process;
	m_process = 0;
}
