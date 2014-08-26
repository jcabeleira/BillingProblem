#ifndef EVENTRECEIVER_H
#define EVENTRECEIVER_H

#include <QTcpServer>

class EventReceiver : public QTcpServer
{
	Q_OBJECT

	public:
	EventReceiver(QObject *parent = 0);

	private:
	void incomingConnection(int socket);
	void processJsonEvent(QByteArray &event);

	private slots:
	void eventReceived();

	private:
	static const quint16 DEFAULT_RECEPTION_PORT = 8080;
};

#endif // EVENTRECEIVER_H
