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

#ifndef QMETACLIENT_H
#define QMETACLIENT_H

#include <QObject>
#include "global.h"

class QIODevice;
class QMetaHost;

class METAEXPORT QMetaClient : public QObject
{
    Q_OBJECT
public:
    QMetaClient(QMetaHost *host, QObject *parent = 0);

    void setDevice(QIODevice *d);
    QIODevice *device() const;

protected:
    bool event(QEvent *e);
    
private slots:
    void onReadyRead();

private:
    QMetaHost *_host;
    QIODevice *_device;
    
};

#endif // QMETACLIENT_H
