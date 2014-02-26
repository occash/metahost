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

#ifndef QTCPTRANSPORT_H
#define QTCPTRANSPORT_H

#include "metaglobal.h"

#include <QObject>
#include <QList>

class QTcpServer;
class QTcpSocket;
class QMetaClient;
class QMetaHost;

class METAEXPORT QTcpTransport : public QObject
{
    Q_OBJECT

public:
    QTcpTransport(QMetaHost *host, QTcpServer *server, QObject *parent = 0);
	QTcpTransport(QMetaHost *host, QTcpSocket *client, QObject *parent = 0);
    ~QTcpTransport();

private:
	void _addClient(QTcpSocket *client);
	void _removeClient(QTcpSocket *client);

private slots:
    void handleConnection();
    void handleDisconnection();
    
private:
    typedef QList<QMetaClient *> ClientList;

    QMetaHost *_host;
    QTcpServer *_server;
	QTcpSocket *_peer;
    ClientList _clients;
    
};

#endif // QTCPTRANSPORT_H
