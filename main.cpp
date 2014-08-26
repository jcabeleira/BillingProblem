
#include <QtCore/QCoreApplication>
#include "EventReceiver.h"

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);
	EventReceiver eventReceiver;

	return a.exec();
}
