#include "compile_worker.hpp"

#include <kovanserial/kovan_serial.hpp>
#include <pcompiler/pcompiler.hpp>

#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QDebug>

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

CompileWorker::CompileWorker(const Kiss::KarPtr &archive, KovanSerial *proto, QObject *parent)
	: QThread(parent),
	m_archive(archive),
	m_proto(proto)
{
}

void CompileWorker::run()
{
	m_resultPath = QString();
	m_output = compile();
	
	qDebug() << "Sending finish!";
	if(!m_proto || !m_proto->sendFileActionProgress(1.0, true)) {
		qWarning() << "send terminal file action progress failed.";
	}
}

const Compiler::OutputList &CompileWorker::output() const
{
	return m_output;
}

const QString &CompileWorker::resultPath() const
{
	return m_resultPath;
}

void CompileWorker::progress(double fraction)
{
	qDebug() << "Progress..." << fraction;
	if(!m_proto || !m_proto->sendFileActionProgress(false, fraction)) {
		qWarning() << "send file action progress failed.";
	}
}

Compiler::OutputList CompileWorker::compile()
{
	using namespace Compiler;
	using namespace Kiss;

	QString path = tempPath();
	qDebug() << "Extracting to" << path;
	
	Cleaner cleaner(path);
	if(!m_archive->extract(path)) {
		return OutputList() << Output(path, 1,
			QByteArray(), "error: Failed to extract KISS Archive.");
	}
	
	QStringList extracted;
	foreach(const QString& file, m_archive->files()) extracted << path + "/" + file;
	
	Engine engine(Compilers::instance()->compilers());
	Options opts = Options::load(QDir::current().filePath("platform.hints"));
	opts.replace("${PREFIX}", QDir::currentPath() + "/prefix");
	Compiler::OutputList ret = engine.compile(Input::fromList(extracted), opts, this);
	
	QStringList terminals;
	bool success = true;
	foreach(const Output& out, ret) {
		if(out.isTerminal() && out.generatedFiles().size() == 1) {
			terminals << out.generatedFiles()[0];
		}
		success &= out.isSuccess();
	}

	if(!success) return ret;

	if(terminals.isEmpty()) {
		ret << Output(path, 1,
			QByteArray(), "error: No terminals detected from compilation.");
		return ret;
	}
	if(terminals.size() > 1) {
		ret << Output(path, 0,
			"warning: Terminal ambiguity in compilation. " 
			"Running the ouput of this compilation is undefined.", QByteArray());
	}

	const QString cachedResult = QDir::tempPath() + "/computer_executable";
	QFile::remove(cachedResult);
	if(!QFile::copy(terminals[0], cachedResult)) {
		ret << OutputList() << Output(path, 1,
			QByteArray(), ("error: Failed to copy \"" + terminals[0]
			+ "\" to \"" + cachedResult + "\"").toLatin1());
		return ret;
	}
	
	m_resultPath = cachedResult;

	return ret;
}

QString CompileWorker::tempPath()
{
	return QDir::tempPath() + "/" + QDateTime::currentDateTime().toString("yyMMddhhmmss") + ".computer";
}
