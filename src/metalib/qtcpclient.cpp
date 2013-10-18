#include "qtcpclient.h"
#include "qmetaevent.h"
#include "qmetahost.h"

#include <QTcpSocket>
#include <QCoreApplication>

QTcpClient::QTcpClient(QMetaHost *host, QObject *parent) :
    QMetaClient(host, parent)
{
}

QTcpClient::~QTcpClient()
{
    
}

void QTcpClient::setSocket(int descriptor)
{
    QTcpSocket *client = new QTcpSocket(this);
    client->setSocketDescriptor(descriptor);

    setDevice(client);
}
