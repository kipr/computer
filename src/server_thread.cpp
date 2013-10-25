#include "server_thread.hpp"

#include "compile_worker.hpp"

#include <kovanserial/tcp_server.hpp>
#include <kovanserial/kovan_serial.hpp>
#include <kovanserial/command_types.hpp>
#include <kovanserial/general.hpp>
#include <kovanserial/platform_defines.hpp>

#include <QDebug>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unistd.h>

#include <pcompiler/pcompiler.hpp>
#include <QDir>

#ifdef Q_OS_WIN
#define NOMINMAX
#include <windows.h>
#endif

ServerThread::ServerThread(TcpServer *server)
	: m_stop(false),
	m_server(server),
	m_transport(new TransportLayer(m_server)),
	m_proto(new KovanSerial(m_transport))
{
}

ServerThread::~ServerThread()
{
	delete m_proto;
	delete m_transport;
	delete m_server;
}

void ServerThread::stop()
{
	m_stop = true;
}

void ServerThread::run()
{
	Packet p;
	while(!m_stop) {
#ifdef Q_OS_WIN
		Sleep(1000);
#else
		sleep(1);
#endif
		QThread::yieldCurrentThread();
		if(!m_server->accept(3)) continue;
		for(;;) {
			const TransportLayer::Return ret = m_proto->next(p, 5000);
			if(ret == TransportLayer::Success && handle(p)) continue;
			if(ret == TransportLayer::UntrustedSuccess && handleUntrusted(p)) continue;
			break;
		}
	}
}

void ServerThread::setUserRoot(const QString &userRoot)
{
	m_userRoot = userRoot;
}

const QString &ServerThread::userRoot() const
{
	return m_userRoot;
}

void ServerThread::setPassword(const QString &password)
{
	if(password.isEmpty()) m_proto->setNoPassword();
	else m_proto->setPassword(password.toStdString());
}

bool ServerThread::handle(const Packet &p)
{
	if(p.type == Command::KnockKnock) m_proto->whosThere();
	else if(p.type == Command::RequestProtocolVersion) m_proto->sendProtocolVersion();
	else if(p.type == Command::FileHeader) handleArchive(p);
	else if(p.type == Command::FileAction) handleAction(p);
	else if(p.type == Command::Hangup) return false;
	return true;
}

bool ServerThread::handleUntrusted(const Packet &p)
{
	if(p.type == Command::KnockKnock) m_proto->whosThere();
	else if(p.type == Command::RequestProtocolVersion) m_proto->sendProtocolVersion();
	else if(p.type == Command::RequestAuthenticationInfo) {
		m_proto->sendAuthenticationInfo(m_proto->isPassworded());
	} else if(p.type == Command::RequestAuthentication) {
		Command::RequestAuthenticationData data;
		p.as(data);
		const bool valid = memcmp(data.password, m_proto->passwordMd5(), 16) == 0;
		m_proto->confirmAuthentication(valid);
	} else if(p.type == Command::Hangup) return false;
	else if(m_password.isEmpty()) return handle(p);
	return true;
}

void ServerThread::handleArchive(const Packet &headerPacket)
{
	quint64 start = msystime();
	
	Command::FileHeaderData header;
	headerPacket.as(header);
	const QString name = header.dest;
	const bool good = QString(header.metadata) == "kar"
		&& !name.isEmpty()
		&& !m_userRoot.isEmpty();
	
	std::ostringstream file(std::ios::binary);

	if(!m_proto->confirmFile(good)) return;
	if(!good) return;
	
	emit stateChanged(tr("Receiving Program..."));
	
	if(!m_proto->recvFile(header.size, &file, 1000)) {
		qWarning() << "recvFile failed";
		return;
	}
	
	quint64 end = msystime();
	qDebug() << "Header size: " << header.size;
	qDebug() << "Took" << (end - start) << "milliseconds to recv";
	
	// Load up the archive
	kiss::Kar *archive = new kiss::Kar();
	std::string data = file.str();
	QByteArray arr(data.c_str(), data.size());
	QDataStream stream(arr);
	stream >> *archive;
	if(!QDir(m_userRoot + "/archives").exists()) QDir(m_userRoot + "/archives").mkpath(".");
	if(!archive->save(QDir(m_userRoot + "/archives").filePath(name))) {
		qWarning() << "Failed to save archive to " << QDir(m_userRoot + "/archives").filePath(name);
	}
	delete archive;

	emit stateChanged(tr("Received Program."));
}

void ServerThread::handleAction(const Packet &action)
{
	using namespace Compiler;
	
	Command::FileActionData data;
	action.as(data);
	const QString type = data.action;
	const QString name = data.dest;
	
	const bool good = !name.isEmpty();
	if(!m_proto->confirmFileAction(good) || !good) return;

	if(type == COMMAND_ACTION_COMPILE) {
		const QString archivePath = m_userRoot + "/archives/" + name;
		const kiss::KarPtr archive = kiss::Kar::load(archivePath);
		
		QFile file(":/target.c");
		if(!file.open(QIODevice::ReadOnly)) {
			qWarning() << "Failed to inject target.c";
		} else {
			archive->setFile("__internal_target___.c", file.readAll());
			file.close();
		}
		
		
		OutputList output;
		CompileWorker *worker = 0;
		if(!archive.isNull()) {
			worker = new CompileWorker(archive, m_proto, this);
			worker->setUserRoot(m_userRoot);
			worker->start();
			worker->wait();
			output = worker->output();
		} else {
			qDebug() << "Failed to load archive";
			output << Output(archivePath, 1, QByteArray(),
				"error: unable to load archive to extract");
			m_proto->sendFileActionProgress(true, 1.0);
		}
		
		output << RootManager(m_userRoot).install(output, name);
				
		if(worker) worker->cleanup();
		
		QByteArray data;
		QDataStream stream(&data, QIODevice::WriteOnly);
		stream << output;
		
		std::istringstream sstream;
		sstream.rdbuf()->pubsetbuf(data.data(), data.size());
		if(!m_proto->sendFile("", "col", (unsigned char *)data.data(), data.size())) {
			qWarning() << "Sending result failed";
			return;
		}
		
		delete worker;
	} else if(type == COMMAND_ACTION_RUN) {
		m_proto->sendFileActionProgress(true, 1.0);
		emit run(name);
	}
}
