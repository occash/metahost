#include <QApplication>

#include "ServerWidget.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	ServertWidget w;
	w.show();

	return a.exec();
}
