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

#include "qmetaclient.h"
#include "qmetaevent.h"
#include "qmetahost.h"

#include <QIODevice>
#include <QCoreApplication>
#include <QDebug>

QMetaClient::QMetaClient(QMetaHost *host, QObject *parent) :
    QObject(parent),
    _host(host),
    _device(nullptr)
{
}

bool QMetaClient::event(QEvent *e)
{
    if(e->type() == MetaEventType)
    {
        QMetaEvent *me = static_cast<QMetaEvent *>(e);

        if(_device) {
            quint16 packetSize = *((quint16 *)me->data());
            _device->write(me->data(), packetSize + sizeof(quint16));
        }

        return true;
    }

    return false;
}

void QMetaClient::onReadyRead()
{
    while(_device->bytesAvailable())
    {
        quint16 packetSize = 0;
        char *input = reinterpret_cast<char *>(&packetSize);
        if(_device->read(input, sizeof(quint16)) == -1)
            break;

        //TODO: read partially
        if(!packetSize || packetSize > _device->bytesAvailable())
            return;

        char *data = new char[packetSize];
        if(!data)
        {
            qWarning() << "Cannot allocate buffer for message";
            return;
        }

        if(_device->read(data, packetSize) == packetSize)
        {
            QMetaEvent *event = new QMetaEvent(this, data);
            QCoreApplication::postEvent(_host, event, Qt::HighEventPriority);
        }
    }
}

void QMetaClient::setDevice(QIODevice *d)
{
    _device = d;
    _device->setParent(this);
    connect(_device, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
}

QIODevice * QMetaClient::device() const
{
    return _device;
}
