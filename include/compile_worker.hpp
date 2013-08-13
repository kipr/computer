#ifndef _COMPILE_WORKER_HPP_
#define _COMPILE_WORKER_HPP_

#include <QThread>

#include <kar/kar.hpp>
#include <pcompiler/output.hpp>
#include <pcompiler/progress.hpp>

class KovanSerial;

class CompileWorker : public QThread, public Compiler::Progress
{
Q_OBJECT
public:
	CompileWorker(const kiss::KarPtr &archive, KovanSerial *proto, QObject *parent = 0);
	
	void run();
	
	const Compiler::OutputList &output() const;
	void setUserRoot(const QString &userRoot);
	const QString &userRoot() const;
	
	void progress(double fraction);
	
	void cleanup();
	
private:
	
	Compiler::OutputList compile();
	static QString tempPath();
	
	kiss::KarPtr m_archive;
	KovanSerial *m_proto;
	Compiler::OutputList m_output;
	QString m_userRoot;
	QString m_tempDir;
};

#endif
