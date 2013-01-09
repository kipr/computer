#ifndef _COMPILE_WORKER_HPP_
#define _COMPILE_WORKER_HPP_

#include <QThread>

#include <kar.hpp>
#include <pcompiler/output.hpp>
#include <pcompiler/progress.hpp>

class KovanSerial;

class CompileWorker : public QThread, public Compiler::Progress
{
public:
	CompileWorker(const Kiss::KarPtr &archive, KovanSerial *proto, QObject *parent = 0);
	
	void run();
	
	const Compiler::OutputList &output() const;
	const QString &resultPath() const;
	
	void progress(double fraction);
	
private:
	
	Compiler::OutputList compile();
	static QString tempPath();
	
	Kiss::KarPtr m_archive;
	KovanSerial *m_proto;
	Compiler::OutputList m_output;
	QString m_resultPath;
};

#endif
