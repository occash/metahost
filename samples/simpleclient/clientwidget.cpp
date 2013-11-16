#include "clientwidget.h"

#include <qmetaclient.h>
#include <qmetahost.h>

#include <QtNetwork/QTcpSocket>
#include <QHostAddress>
#include <QPushButton>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QDebug>

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
    
	host = new QMetaHost();
    host->registerClass<QPushButton>();
    transport = new QMetaClient(host, this);
    transport->setDevice(client);

	serverObj = host->getObject("ServerButton", transport);
	connect(serverObj, SIGNAL(clicked()), this, SLOT(onServerButtonClicked()));

    QObject *test = host->getObject("TestObject", transport);
    int ret;
    QMetaObject::invokeMethod(test, "test_params", Q_RETURN_ARG(int, ret), Q_ARG(int, 1), Q_ARG(qreal, 2.0));
    qDebug() << test->property("string").toString();
    test->setProperty("string", "Trololo");
    qDebug() << test->property("string").toString();
}

void ClientWidget::onServerButtonClicked()
{
	QMessageBox::warning(this, "Server message", "Button on server side was clicked.");
}
