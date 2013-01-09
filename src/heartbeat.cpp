#include "heartbeat.hpp"

#include <QTimer>

Heartbeat::Heartbeat(QObject *parent)
	: QObject(parent),
	m_advertiser(true)
{
	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), SLOT(beat()));
	timer->start(3000);
}

Heartbeat::~Heartbeat()
{
	
}

void Heartbeat::setAdvert(const Advert &advert)
{
	m_advert = advert;
}

const Advert &Heartbeat::advert() const
{
	return m_advert;
}

void Heartbeat::beat()
{
	m_advertiser.reset();
	m_advertiser.pulse(m_advert);
}