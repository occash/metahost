#ifndef CLIENTWIDGET_H
#define CLIENTWIDGET_H

#include <QWidget>

class QTcpSocket;
class QTcpTransport;
class QMetaHost;

class QLineEdit;
class QPushButton;

class ClientWidget : public QWidget
{
	Q_OBJECT

public:
	ClientWidget();
	~ClientWidget();

private:
	QTcpSocket *client;
	QTcpTransport *transport;
	QMetaHost *host;
	QObject *serverObj;

	QLineEdit *ipAddress;
	QPushButton *button;

private slots:
	void onConnectClicked();
	void onServerButtonClicked();

};

#endif