#include "qtcptransport.h"
#include "qmetahost.h"

#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QTcpServer>
#include <QDebug>
#include <QByteArray>
#include <QDataStream>

QTcpTransport::QTcpTransport(const QHostAddress& address, quint32 port, QObject *parent) :
    QObject(parent),
    _server(new QTcpServer(this))
{
    connect(_server, SIGNAL(newConnection()), this, SLOT(handleConnection()));

    qDebug() << "Starting tcp server...";
    if(_server->listen(address, port))
        qDebug() << "Server started.";
    else
        qCritical() << "Server have not been started.";
}

QTcpTransport::~QTcpTransport()
{
    _server->close();
    qDeleteAll(_clients);
    qDebug() << "Server stopped.";
}

void QTcpTransport::broadcast(const QByteArray &data)
{
    QByteArray dataToSend;
    QDataStream out(&dataToSend, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_8);

    out << dataToSend.size();
    out << data;

    foreach(QIODevice *client, _clients)
    {
        client->write(dataToSend);
    }
}

void QTcpTransport::write(QIODevice *client, const QByteArray &data)
{
    //remove duplication
    QByteArray dataToSend = pack(data);

    client->write(dataToSend);
}

void QTcpTransport::handleConnection()
{
    while(_server->hasPendingConnections()) {
        QTcpSocket *client = _server->nextPendingConnection();

        //***********Extract AddClient() method**********************
        connect(client, SIGNAL(disconnected()), this, SLOT(handleDisconnection()));
        connect(client, SIGNAL(readyRead()), this, SLOT(dispatch()));
        _clients.append(client);
        qDebug() << "New client arrived:" << client->peerAddress();
        //**************************************************
    }
}

void QTcpTransport::handleDisconnection()
{
    QTcpSocket *client = qobject_cast<QTcpSocket *>(sender());
    if(!client)
        return;

    //**********Extract removeClient() method*****************
    _clients.removeAll(client);
    client->deleteLater();
    qDebug() << "Client disconnected:" << client->peerAddress();
    //******************************************************
}

void QTcpTransport::dispatch()
{
    QTcpSocket *client = qobject_cast<QTcpSocket *>(sender());
    if(!client)
        return;

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

    QMetaHost *host = qobject_cast<QMetaHost *>(parent());
    if(!host)
        return;

    host->processCommand(client, &data);
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

