#include "serverwidget.h"
#include "moctest.h"

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
    button->setDefault(true);
	layout->addWidget(button);
	setLayout(layout);
    test = new MocTest(this);
    test->setObjectName("TestObject");
    test->setString("Ololo");
    connect(test, SIGNAL(stringChanged()), this, SLOT(onPropChange()));

	//Create host
	server = new QTcpServer(this);
	server->listen(QHostAddress::Any, 6565);
	
	host = new QMetaHost();
    transport = new QTcpTransport(host, server);

	host->registerObject(button);
    host->registerObject(test);
}

ServertWidget::~ServertWidget()
{

}

void ServertWidget::onPropChange()
{
    qDebug() << test->getString();
}
