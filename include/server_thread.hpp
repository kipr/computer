#ifndef _SERVER_THREAD_HPP_
#define _SERVER_THREAD_HPP_

#include <QObject>
#include <QString>
#include <QRunnable>

#include <kar/kar.hpp>
#include <kovanserial/transport_layer.hpp>

class TcpServer;
class KovanSerial;

class ServerThread : public QObject, public QRunnable
{
Q_OBJECT
public:
	ServerThread(TcpServer *server);
	~ServerThread();
	
	void stop();
	virtual void run();
	
	void setUserRoot(const QString &userRoot);
	const QString &userRoot() const;
	
	void setPassword(const QString &password);
	
signals:
	void stateChanged(const QString &state);
	void run(const QString &executable);
	
private:
	bool handle(const Packet &p);
	bool handleUntrusted(const Packet &p);
	void handleArchive(const Packet &headerPacket);
	void handleAction(const Packet &action);
	
	bool m_stop;
	TcpServer *m_server;
	TransportLayer *m_transport;
	KovanSerial *m_proto;
	
	QString m_userRoot;
	QString m_password;
};

#endif
