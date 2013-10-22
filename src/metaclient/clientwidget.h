#ifndef CLIENTWIDGET_H
#define CLIENTWIDGET_H

#include <QWidget>

class QTcpSocket;
class QMetaClient;
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
	QMetaClient *transport;
	QMetaHost *host;
	QObject *serverObj;

	QLineEdit *ipAddress;
	QPushButton *button;

private slots:
	void onConnectClicked();
	void onServerButtonClicked();

};

#endif