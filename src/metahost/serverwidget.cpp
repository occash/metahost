#include "serverwidget.h"

#include <qtcptransport.h>
#include <qmetahost.h>

#include <QtNetwork/QTcpServer>
#include <QPushButton>
#include <QVBoxLayout>

ServertWidget::ServertWidget()
{
	QVBoxLayout *layout = new QVBoxLayout(this);
	button = new QPushButton("Click", this);
    button->setObjectName("ServerButton");
	layout->addWidget(button);
	setLayout(layout);

	//Create host
	server = new QTcpServer(this);
	server->listen(QHostAddress::Any, 6565);
	
	host = new QMetaHost();
    transport = new QTcpTransport(host, server);

	host->registerObject(button);
}

ServertWidget::~ServertWidget()
{

}