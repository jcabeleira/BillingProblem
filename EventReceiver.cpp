#include "EventReceiver.h"
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QFile>


EventReceiver::EventReceiver(QObject *parent) : QTcpServer(parent)
{
	processTwilioCSV();

	listen(QHostAddress::Any, DEFAULT_RECEPTION_PORT);
}


void EventReceiver::processTwilioCSV()
{
	QFile file("prices.csv");
	if(file.open(QIODevice::ReadOnly |  QIODevice::Text))
	{
		//Ignore the first line because it's just the description header.
		file.readLine();
		QString line;

		//For each line in the file.
		while(file.atEnd() == false)
		{
			//Split the fields by their commas.
			line = file.readLine();
			QStringList fields = line.remove("\"").remove(" ").split(',');

			//Get the price for this group of call codes.
			float price = fields[1].toFloat();

			//For each code in the call code group...
			for(unsigned int field = 2; field < fields.size(); field++)
			{
				//Add the call code and the corresponding price to the prices list.
				QString callCode = fields[field];
				prices.append(CallCodePrice(callCode, price));
			}
		}
	}
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


void EventReceiver::processJsonEvent(QByteArray &eventData)
{
	QJsonDocument jsonEvent = QJsonDocument::fromJson(eventData);
	QJsonObject jsonData = jsonEvent.object();
	QString eventName = jsonData["event"].toString();

	if(eventName == "call_finished")
	{
		QString talkdeskAccount = jsonData["account_id"].toString();
		QString talkDeskPhoneNumber = jsonData["talkdesk_phone_number"].toString();
		QString externalPhoneNumber = jsonData["forwarded_phone_number"].toString();
		int duration = jsonData["duration"].toString().toInt();
		int cost = calculateCost(talkdeskAccount, talkDeskPhoneNumber, externalPhoneNumber, duration);

		updateUserBillingInformation(talkdeskAccount, talkDeskPhoneNumber, externalPhoneNumber, duration, cost);
	}
}


int EventReceiver::calculateCost(QString talkdeskAccount, QString talkDeskPhoneNumber, QString externalPhoneNumber, int duration)
{
	int durationInMinutes = (duration / 60) + 1;
	int talkdeskCostPerMinute = calculateTalkdeskNumberCost(talkDeskPhoneNumber);
	int externalCostPerMinute = calculateExternalNumberCost(externalPhoneNumber);
	int totalCost = (talkdeskCostPerMinute + externalCostPerMinute + PROFIT_MARGIN_PER_MINUTE) * durationInMinutes;

	return totalCost;
}


int EventReceiver::calculateTalkdeskNumberCost(QString talkDeskPhoneNumber)
{
	int talkDeskCostPerMinute = 0;

	//Is a toll free UK number?
	if(talkDeskPhoneNumber.startsWith("+440800") || talkDeskPhoneNumber.startsWith("+440808"))
	{
		talkDeskCostPerMinute = 6;
	}
	//Is a toll free US number?
	else if(talkDeskPhoneNumber.startsWith("+1800") || talkDeskPhoneNumber.startsWith("+1888") || talkDeskPhoneNumber.startsWith("+1877") ||
			talkDeskPhoneNumber.startsWith("+1866") || talkDeskPhoneNumber.startsWith("+1855") || talkDeskPhoneNumber.startsWith("+1844"))
	{
		talkDeskCostPerMinute = 3;
	}
	else
	{
		talkDeskCostPerMinute = 1;
	}

	return talkDeskCostPerMinute;
}


int EventReceiver::calculateExternalNumberCost(QString externalPhoneNumber)
{
	int externalCostPerMinute = 0;

	//If the external phone number is empty then the call was answered on the browser.
	if(externalPhoneNumber.isEmpty())
	{
		externalCostPerMinute = 1;
	}
	else
	{
		QString phoneNumberWithoutPlusSign = externalPhoneNumber.remove("+");
		int bestMatch = 0;

		//Find the call code in the prices list.
		for(unsigned int price = 0; price < prices.size(); price++)
		{
			//Did we find a matching call code in our price list?
			if(phoneNumberWithoutPlusSign.startsWith(prices[price].first))
			{
				//Do we have a better match?
				//For instance the number "3516XXXXXXXXX" matches the call codes "351" and "3516" in our price list, the correct match
				//is the one that uses more digits.
				if(prices[price].first.length() > bestMatch)
				{
					//Convert the price from dolars to cents.
					//TODO: possible precision loss, prices values can be bellow cents precision. This should be improved.
					externalCostPerMinute = prices[price].second * 100;
					bestMatch = prices[price].first.length();
				}
			}
		}
	}

	return externalCostPerMinute;
}


void EventReceiver::updateUserBillingInformation(QString talkdeskAccount, QString talkdeskPhoneNumber, QString externalPhoneNumber, int callDuration, int callCost)
{
	QString userFileName = "userDatabase\\" + talkdeskAccount;
	QFile file(userFileName);
	
	QJsonObject inputJsonObject;
	QJsonDocument inputJsonDocument(inputJsonObject);

	if(file.exists())
	{
		file.open(QIODevice::ReadWrite);
	
		QByteArray fileContents = file.readAll();
		inputJsonDocument = QJsonDocument::fromJson(fileContents);
		inputJsonObject = inputJsonDocument.object();

		//Clear the file contents.
		file.resize(0);
	}
	else
	{
		file.open(QIODevice::WriteOnly);

		inputJsonObject["account_id"] = talkdeskAccount;
		inputJsonObject["calls"] = QJsonArray();
	}

	//Copy the basic user information.
	QJsonObject outputObject;
	outputObject["account_id"] = inputJsonObject["account_id"];

	//Update the calls array.
	QJsonObject callInformation;
	callInformation["talkdesk_phone_number"] = talkdeskPhoneNumber;
	callInformation["forwarded_phone_number"] = externalPhoneNumber.isEmpty() ? QJsonValue::Null : QJsonValue(externalPhoneNumber);
	callInformation["duration"] = callDuration;
	callInformation["cost"] = callCost;
	
	QJsonArray callsArray = inputJsonObject["calls"].toArray();
	callsArray.append(callInformation);
	outputObject["calls"] = callsArray;
	
	QJsonDocument outputJsonDocument(outputObject);
	file.write(outputJsonDocument.toJson());
}