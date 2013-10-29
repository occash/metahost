#ifndef SERVERWIDGET_H
#define SERVERWIDGET_H

#include <QWidget>

class QTcpServer;
class QTcpTransport;
class QMetaHost;
class MocTest;
class QPushButton;

class ServertWidget : public QWidget
{
	Q_OBJECT

public:
	ServertWidget();
	~ServertWidget();

private:
	QTcpServer *server;
	QTcpTransport *transport;
	QMetaHost *host;
    MocTest *test;
	QPushButton *button;

};

#endif