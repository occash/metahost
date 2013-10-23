#include "qtcptransport.h"
#include "qmetahost.h"
#include "qmetaclient.h"

#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QTcpServer>
#include <QByteArray>
#include <QDataStream>

#include <QDebug>

QTcpTransport::QTcpTransport(QMetaHost *host, QTcpServer *server, QObject *parent) :
    QObject(parent),
    _host(host),
    _server(server),
	_peer(nullptr)
{
    connect(_server, SIGNAL(newConnection()), this, SLOT(handleConnection()));
}

QTcpTransport::QTcpTransport(QMetaHost *host, QTcpSocket *client, QObject *parent) :
    QObject(parent),
    _host(host),
	_server(nullptr),
	_peer(client)
{
	_addClient(client);
}

QTcpTransport::~QTcpTransport()
{
	if(_server)
		_server->close();

    qDeleteAll(_clients);
}

void QTcpTransport::handleConnection()
{
    while(_server->hasPendingConnections()) {
        QTcpSocket *client = _server->nextPendingConnection();

        _addClient(client);
    }
}

void QTcpTransport::handleDisconnection()
{
    QTcpSocket *client = qobject_cast<QTcpSocket *>(sender());
    if(!client)
        return;

    _removeClient(client);
}

void QTcpTransport::_addClient(QTcpSocket *client)
{
	connect(client, SIGNAL(disconnected()), this, SLOT(handleDisconnection()));

    QMetaClient *metaClient = new QMetaClient(_host, this);
    metaClient->setDevice(client);
	_clients.append(metaClient);

	qDebug() << "New client arrived:" << client->peerAddress();
}

void QTcpTransport::_removeClient(QTcpSocket *client)
{
    foreach(QMetaClient *metaClient, _clients)
    {
        if(metaClient->device() == client)
        {
            int id = _clients.indexOf(metaClient);
            metaClient->deleteLater();
            _clients.removeAt(id);
        }
    }

    qDebug() << "Client disconnected:" << client->peerAddress();
}

