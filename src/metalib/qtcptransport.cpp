#include "qtcptransport.h"

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
    QByteArray dataToSend;
    QDataStream out(&dataToSend, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_8);

    out << dataToSend.size();
    out << data;

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

    quint16 size;
    QByteArray data;
    QDataStream in(client);
    in.setVersion(QDataStream::Qt_4_8);
    in >> size;
    if(client->bytesAvailable() < size)
    {
        //Make appropriate logging
        qDebug() << "Error: Number of bytes manipulated!";
        return;
    }

    in >> data;
    qDebug() << "Data recieved:" << data;

    //process further
}

