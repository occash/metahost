/*********************************************************************
This file is part of the MetaHost library.
Copyright (C) 2013 Artem Shal
artiom.shal@gmail.com

The MetaHost library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

This software is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
USA.
**********************************************************************/

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

