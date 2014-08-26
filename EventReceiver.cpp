#include "EventReceiver.h"
#include <QTcpSocket>
#include <iostream>

EventReceiver::EventReceiver(QObject *parent) : QTcpServer(parent)
{
	listen(QHostAddress::Any, DEFAULT_RECEPTION_PORT);
}


void EventReceiver::incomingConnection(int socket)
{
	QTcpSocket* tcpSocket = new QTcpSocket(this);
	tcpSocket->setSocketDescriptor(socket);

	connect(tcpSocket, SIGNAL(readyRead()), this, SLOT(eventReceived()));
}


void EventReceiver::eventReceived()
{
	QTcpSocket* socket = (QTcpSocket*) sender();
	QByteArray httpData = socket->readAll();

	int startOfPayload = httpData.indexOf("\r\n\r\n") + 4;
	QByteArray httpPayloadData = httpData.mid(startOfPayload);

	processJsonEvent(httpPayloadData);
}


void EventReceiver::processJsonEvent(QByteArray &event)
{
}