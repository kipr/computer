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
		qDebug() << "Waiting for connections...";
		if(!m_server->accept(1000)) continue;
		while(m_proto->next(p, 5000) && handle(p));
	}
}

bool ServerThread::handle(const Packet &p)
{
	if(p.type == Command::FileHeader) handleArchive(p);
	else if(p.type == Command::FileAction) handleAction(p);
	else if(p.type == Command::Hangup) return false;
	return true;
}

void ServerThread::handleArchive(const Packet &headerPacket)
{
	quint64 start = msystime();
	
	Command::FileHeaderData header;
	headerPacket.as(header);
	const bool good = QString(header.metadata) == "kar";
	
	std::ostringstream file(std::ios::binary);

	if(!m_proto->confirmFile(good)) return;
	if(!good) return;
	
	emit stateChanged(tr("Receiving Program..."));
	
	if(!m_proto->recvFile(header.size, &file, 1000)) {
		qWarning() << "recvFile failed";
		return;
	}
	
	quint64 end = msystime();
	qDebug() << "Took" << (end - start) << "milliseconds to recv";
	
	std::string data = file.str();
	
	QByteArray arr(data.c_str(), data.size());
	QDataStream stream(arr);
	
	Kiss::Kar *archive = new Kiss::Kar();
	stream >> *archive;
	m_archive = Kiss::KarPtr(archive);
	m_archiveLocation = header.dest;
	qDebug() << "archiveLocation" << m_archiveLocation;
	m_executable = QString();

	emit stateChanged(tr("Received Program."));
}

void ServerThread::handleAction(const Packet &action)
{
	Command::FileActionData data;
	action.as(data);
	// Check that the archive we have is the one that KISS
	// wants us to act on.
	const bool good = m_archiveLocation == data.dest;
	if(!m_proto->confirmFileAction(good)) return;
	if(!good) return;
	
	QString type = data.action;

	if(type == COMMAND_ACTION_COMPILE) {

		CompileWorker *worker = new CompileWorker(m_archive, m_proto, this);
		worker->start();
		worker->wait();
		
		qDebug() << "Sending results...";
		QByteArray data;
		QDataStream stream(&data, QIODevice::WriteOnly);
		stream << worker->output();
		
		std::istringstream sstream;
		sstream.rdbuf()->pubsetbuf(data.data(), data.size());
		if(!m_proto->sendFile("", "col", &sstream)) {
			qWarning() << "Sending result failed";
			return;
		}
		
		const QString &resultPath = worker->resultPath();
		if(resultPath.isEmpty()) return;
		
		m_executable = resultPath;
	} else if(type == COMMAND_ACTION_RUN) {
		m_proto->sendFileActionProgress(true, 1.0);
		if(m_executable.isEmpty()) return;
		emit run(m_executable);
	}
}
