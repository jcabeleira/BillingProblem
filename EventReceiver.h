#ifndef EVENTRECEIVER_H
#define EVENTRECEIVER_H

#include <QTcpServer>
#include <QVector>

class EventReceiver : public QTcpServer
{
	Q_OBJECT

	public:
	EventReceiver(QObject *parent = 0);

	private:
	void processTwilioCSV();
	void incomingConnection(int socket);
	void processJsonEvent(QByteArray &eventData);
	int calculateCost(QString talkdeskAccount, QString talkDeskPhoneNumber, QString externalPhoneNumber, int duration);
	int calculateTalkdeskNumberCost(QString talkDeskPhoneNumber);
	int calculateExternalNumberCost(QString externalPhoneNumber);
	void updateUserBillingInformation(QString talkdeskAccount, QString talkdeskPhoneNumber, QString externalPhoneNumber, int callDuration, int callCost);

	private slots:
	void eventReceived();

	private:
	static const quint16 DEFAULT_RECEPTION_PORT = 8080;
	static const int PROFIT_MARGIN_PER_MINUTE = 0;
	typedef QPair<QString, float> CallCodePrice;

	QVector<CallCodePrice> prices;
};

#endif // EVENTRECEIVER_H
