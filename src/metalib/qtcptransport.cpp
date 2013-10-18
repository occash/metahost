#include "qtcptransport.h"
#include "qmetahost.h"

#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QTcpServer>
#include <QDebug>
#include <QByteArray>
#include <QDataStream>

QTcpTransport::QTcpTransport(QTcpServer *server, QObject *parent) :
    QObject(parent),
    _server(server),
	_peer(nullptr)
{
    connect(_server, SIGNAL(newConnection()), this, SLOT(handleConnection()));
}

QTcpTransport::QTcpTransport(QTcpSocket *client, QObject *parent) :
QObject(parent),
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
    qDebug() << "Server stopped.";
}

void QTcpTransport::broadcast(const QByteArray &data)
{
    QByteArray dataToSend = pack(data);

    foreach(QIODevice *client, _clients)
    {
        client->write(dataToSend);
    }

    qDebug() << "Sent:" << dataToSend;
}

void QTcpTransport::write(QIODevice *client, const QByteArray &data)
{
    //remove duplication
    QByteArray dataToSend = pack(data);

    client->write(dataToSend);

    qDebug() << "Sent:" << dataToSend;
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

void QTcpTransport::dispatch()
{
    QTcpSocket *client = qobject_cast<QTcpSocket *>(sender());
    if(!client)
        return;


    while(client->bytesAvailable())
    {
        QDataStream in(client);
        in.setVersion(QDataStream::Qt_4_8);

        quint16 size = 0;
        in >> size;
        qDebug() << "Bytes available" << client->bytesAvailable();
        qDebug() << "Data expected:" << size;

        if(client->bytesAvailable() < size)
        {
            //Make appropriate logging
            qDebug() << "Error: Number of bytes manipulated!";
            return;
        }

        QByteArray data;
        in >> data;
        qDebug() << "Data received:" << data;
        qDebug() << data.size();

        qDebug() << "Received:" << data;

        QMetaHost *host = qobject_cast<QMetaHost *>(parent());
        if(!host)
            return;

        host->processCommand(client, &data);
    }
}

QByteArray QTcpTransport::pack( const QByteArray& data )
{
    QByteArray packet;
    QDataStream out(&packet, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_8);
    out << quint16(0);
    out << data;
    out.device()->seek(0);
    out << (quint16)(packet.size() - sizeof(quint16));

    return packet;
}

void QTcpTransport::_addClient(QTcpSocket *client)
{
	connect(client, SIGNAL(disconnected()), this, SLOT(handleDisconnection()));
	connect(client, SIGNAL(readyRead()), this, SLOT(dispatch()));
	_clients.append(client);
	qDebug() << "New client arrived:" << client->peerAddress();
}

void QTcpTransport::_removeClient(QTcpSocket *client)
{
    _clients.removeAll(client);
    client->deleteLater();
    qDebug() << "Client disconnected:" << client->peerAddress();
}

