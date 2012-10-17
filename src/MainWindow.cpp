#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <easydevice/discovery_constants.hpp>
#include <pcompiler/pcompiler.hpp>

#include "Computer.h"
#include "QUserInfo.h"

#include <QPrintDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QDesktopServices>
#include <QCryptographicHash>
#include <QUrl>
#include <QNetworkInterface>
#include <QFileSystemModel>
#include <QClipboard>
#include <QDebug>

using namespace EasyDevice;

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent),
	m_server(this),
	m_discovery(this),
	m_process(0),
	ui(new Ui::MainWindow),
	m_timer(this),
	m_generator(PasswordGenerator::Numbers | PasswordGenerator::Letters),
	m_filesystemModel(new QFileSystemModel(this))
{
	ui->setupUi(this);
	setWindowTitle(QUserInfo::username() + "'s Computer");
	
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
	
	m_filesystemModel->setFilter(QDir::NoDot | QDir::NoDotDot | QDir::Files);
	m_filesystemModel->setNameFilters(QStringList() << "*.kissar");
	ui->programWidget->hide();
	ui->programList->setModel(m_filesystemModel);
	
	connect(ui->actionPrint, SIGNAL(activated()), this, SLOT(print()));
	connect(ui->actionSave, SIGNAL(activated()), this, SLOT(saveToFile()));
	connect(ui->actionCopy, SIGNAL(activated()), this, SLOT(copy()));
	connect(ui->actionStop, SIGNAL(activated()), this, SLOT(terminateProcess()));
	connect(ui->actionAbout, SIGNAL(activated()), this, SLOT(about()));
	connect(ui->actionSettings, SIGNAL(activated()), this, SLOT(settings()));
	connect(ui->actionOpenWorkingDirectory, SIGNAL(activated()), this, SLOT(openWorkingDir()));
	connect(ui->console, SIGNAL(abortRequested()), this, SLOT(terminateProcess()));
	connect(ui->deleteButton, SIGNAL(clicked()), this, SLOT(deleteSelectedPrograms()));
	connect(ui->runButton, SIGNAL(clicked()), this, SLOT(runSelectedProgram()));
	
	connect(ui->programList->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this, SLOT(programSelectionChanged(QItemSelection)));
	programSelectionChanged(ui->programList->selectionModel()->selection());
	
	updateSettings();
	
	connect(&m_timer, SIGNAL(timeout()), this, SLOT(timeout()));
}

MainWindow::~MainWindow()
{
	killProcess();
	delete ui;
	delete m_filesystemModel;
}

const bool MainWindow::run(const QString& name)
{
	if(m_compileResults.value(name).isEmpty()) compile(name);
	if(m_compileResults.value(name).isEmpty()) return false;
	

	killProcess();
	m_process = new QProcess();
	QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
	// TODO: This will only work on OS X
	env.insert("DYLD_LIBRARY_PATH", QDir::currentPath() + "/prefix/usr/lib:" + env.value("DYLD_LIBRARY_PATH"));
	m_process->setProcessEnvironment(env);
	m_process->setWorkingDirectory(m_workingDirectory.path());
	ui->console->setProcess(m_process);
	connect(m_process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(processFinished()));
	processStarted();
	m_process->start(m_compileResults.value(name), QStringList());
	raise();
	extendTimeout();
	
	return true;
}

struct Cleaner
{
public:
	Cleaner(const QString& path)
		: path(path)
	{
	}
	
	~Cleaner()
	{
		remove(path);
	}
	
private:
	bool remove(const QString& path)
	{
		QDir dir(path);

		if(!dir.exists()) return true;
		
		QFileInfoList entries = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden
			| QDir::AllDirs | QDir::Files, QDir::DirsFirst);
		
		foreach(const QFileInfo& entry, entries) {
			const QString entryPath = entry.absoluteFilePath();
			if(!(entry.isDir() ? remove(entryPath) : QFile::remove(entryPath))) return false;
		}
		
		if(!dir.rmdir(path)) return false;
		
		return true;
	}
	
	QString path;
};

Compiler::OutputList MainWindow::compile(const QString& name)
{
	using namespace Compiler;
	using namespace Kiss;
	
	KarPtr archive = Kar::load(programSavePath(name));
	QString path = tempPath();
	qDebug() << "Extracting to" << path;
	if(!archive->extract(path)) {
		return OutputList() << Output(programSavePath(name), 1,
			QByteArray(), "error: Failed to extract KISS Archive.");
	}
	QStringList extractedFiles;
	foreach(const QString& file, archive->files()) {
		extractedFiles << path + "/" + file;
	}
	Input input = Input::fromList(extractedFiles);
	Engine engine(Compilers::instance()->compilers());
	Options opts = Options::load(QDir::current().filePath("platform.hints"));
	opts.replace("${PREFIX}", QDir::currentPath() + "/prefix");
	qDebug() << "Building with" << opts;
	OutputList ret = engine.compile(input, opts);
	Cleaner cleaner(path);
	unsigned int terminals = 0;
	QString firstTerminalFile;
	bool success = true;
	foreach(const Output& out, ret) {
		if(out.isTerminal() && out.generatedFiles().size() == 1) {
			if(!terminals) firstTerminalFile = out.generatedFiles()[0];
			++terminals;
		}
		success &= out.exitCode() == 0 && out.error().isEmpty();
	}
	
	QFile::remove(m_compileResults.value(name));
	
	if(!success) {
		return ret;
	}
	
	if(!terminals) {
		ret << OutputList() << Output(programSavePath(name), 1,
			QByteArray(), "error: No terminals detected from compilation.");
		return ret;
	}
	if(terminals > 1) {
		ret << Output(programSavePath(name), 0,
			"warning: Terminal ambiguity in compilation. " 
			"Running the ouput of this compilation is undefined.", QByteArray());
	}
	
	QString cachedResult = cachePath(name) + "/" + QFileInfo(firstTerminalFile).fileName();
	QFile::remove(cachedResult);
	if(!QFile::copy(firstTerminalFile, cachedResult)) {
		ret << OutputList() << Output(programSavePath(name), 1,
			QByteArray(), ("error: Failed to copy \"" + firstTerminalFile
			+ "\" to \"" + cachedResult + "\"").toLatin1());
	}
	
	m_compileResults[name] = cachedResult;
	return ret;
}

const bool MainWindow::download(const QString& name, const Kiss::KarPtr& archive)
{
	return archive->save(programSavePath(name));
}

Filesystem *MainWindow::filesystem()
{
	return &m_filesystem;
}

QStringList MainWindow::list() const
{
	
	return QStringList();
}

bool MainWindow::deleteProgram(const QString& name)
{
	return false;
}

QString MainWindow::interaction(const QString& command)
{
	return "Interaction not implemented";
}

const bool MainWindow::isAuthenticated(const QHostAddress& address)
{
	QSettings settings;
	settings.beginGroup(KISS_CONNECTION);
	
	if(QNetworkInterface::allAddresses().contains(address)) {
		m_currentAddress = QHostAddress();
		return true;
	}
	if(settings.value(DISALLOW_REMOTE, false).toBool())
		return false;
	return m_currentAddress == address;
}

const bool MainWindow::authenticationRequest(const QHostAddress& address)
{
	QString string = m_generator.password();
	ui->statusbar->showMessage(tr("Incoming connection. The password is ") + string, 0); // TODO: Make prettier
	QCryptographicHash gen(QCryptographicHash::Sha1);
	gen.addData(string.toUtf8());
	m_hash = gen.result();
	return true;
}

const EasyDevice::ServerDelegate::AuthenticateReturn MainWindow::authenticate(const QHostAddress& address, const QByteArray& hash)
{
	if(m_hash.isNull()) return EasyDevice::ServerDelegate::AuthWillNotAccept;
	if(hash.isNull() || m_hash != hash) return EasyDevice::ServerDelegate::AuthTryAgain;
	m_hash.clear();
	m_currentAddress = address;
	extendTimeout();
	ui->statusbar->showMessage(tr("Paired with ") + m_currentAddress.toString(), 0);
	return EasyDevice::ServerDelegate::AuthSuccess;
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

void MainWindow::timeout()
{
	ui->statusbar->showMessage("A command has not been sent in a while. Relocking.", 0);
	m_currentAddress = QHostAddress();
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

void MainWindow::runSelectedProgram()
{
	QItemSelectionModel *selection = ui->programList->selectionModel();
	QModelIndexList indexes = selection->selectedIndexes();
	if(indexes.size() != 1) return;
	QModelIndex index = indexes[0];
	QString fileName = m_filesystemModel->fileName(index);
	run(QFileInfo(fileName).completeBaseName());
}

void MainWindow::deleteSelectedPrograms()
{
	QItemSelectionModel *selection = ui->programList->selectionModel();
	QModelIndexList indexes = selection->selectedIndexes();
	foreach(const QModelIndex& index, indexes) m_filesystemModel->remove(index);
}

void MainWindow::programSelectionChanged(const QItemSelection& selection)
{
	QModelIndexList indexes = selection.indexes();
	ui->deleteButton->setEnabled(!indexes.isEmpty());
	ui->runButton->setEnabled(indexes.size() == 1);
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

	DeviceInfo deviceInfo = m_discovery.deviceInfo();
	deviceInfo.setDisplayName(displayName());
	m_discovery.setDeviceInfo(deviceInfo);
	
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
	QString path = settings.value(PROGRAM_DIRECTORY, QDir::homePath() + "/" + tr("KISS Programs")).toString();
	QString workPath = settings.value(WORKING_DIRECTORY, QDir::homePath() + "/" + tr("KISS Work Dir")).toString();

	if(!QDir(path).exists()) QDir().mkpath(path);
	
	m_workingDirectory = QDir(workPath);
	if(!m_workingDirectory.exists()) QDir().mkpath(workPath);
	
	m_filesystemModel->setRootPath(path);
	ui->programList->setRootIndex(m_filesystemModel->index(path));
	settings.endGroup();
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

QString MainWindow::tempPath() const
{
	return QDir::tempPath() + "/" + QDateTime::currentDateTime().toString("yyMMddhhmmss") + ".computer";
}

QString MainWindow::cachePath(const QString& name) const
{
	QString ret = programSavePath(name) + ".cache";
	QDir().mkpath(ret);
	return ret;
}

QString MainWindow::programSavePath(const QString& name) const
{
	return QDir(m_filesystemModel->rootPath()).filePath(name + ".kissar");
}
