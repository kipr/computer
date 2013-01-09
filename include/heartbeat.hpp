#ifndef _HEARTBEAT_HPP_
#define _HEARTBEAT_HPP_

#include <QObject>

#include <kovanserial/udp_advertiser.hpp>

class Heartbeat : public QObject
{
Q_OBJECT
public:
	Heartbeat(QObject *parent = 0);
	~Heartbeat();
	
	void setAdvert(const Advert &advert);
	const Advert &advert() const;
	
private slots:
	void beat();
	
private:
	UdpAdvertiser m_advertiser;
	Advert m_advert;
};

#endif
