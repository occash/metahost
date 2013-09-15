#include "clientwidget.h"

#include <qtcptransport.h>
#include <qmetahost.h>

#include <QtNetwork/QTcpSocket>
#include <QHostAddress>
#include <QPushButton>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QMessageBox>

ClientWidget::ClientWidget()
{
	QVBoxLayout *layout = new QVBoxLayout(this);
	ipAddress = new QLineEdit(this);
	button = new QPushButton("Connect", this);
	layout->addWidget(ipAddress);
	layout->addWidget(button);
	setLayout(layout);
	connect(button, SIGNAL(clicked()), this, SLOT(onConnectClicked()));
}

ClientWidget::~ClientWidget()
{

}

void ClientWidget::onConnectClicked()
{
	QHostAddress address(ipAddress->text());
	client = new QTcpSocket(this);
	client->connectToHost(address, 6565, QIODevice::ReadWrite);
	transport = new QTcpTransport(client);
	host = new QMetaHost(transport);

	serverObj = host->getObject("ServerButton");
	connect(serverObj, SIGNAL(clicked()), this, SLOT(onServerButtonClicked()));
}

void ClientWidget::onServerButtonClicked()
{
	QMessageBox::warning(this, "Server message", "Button on server side was clicked.");
}
