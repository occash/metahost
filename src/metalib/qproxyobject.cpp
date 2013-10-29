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

#include "qproxyobject.h"
#include "qmetahost.h"

#include <QDebug>
#include <QMetaObject>

QProxyObject::QProxyObject(QMetaHost *host, QObject *parent) :
    QObject(parent),
    _meta(nullptr),
    _host(host)
{
}

const QMetaObject *QProxyObject::metaObject() const
{
	return _meta;
}

void *QProxyObject::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;

    const QMetaObject *meta = _meta;
    while(meta)
    {
        if (!strcmp(_clname, meta->d.stringdata))
            return static_cast<void *>(const_cast<QProxyObject *>(this));

        meta = meta->d.superdata;
    }
    
    return QObject::qt_metacast(_clname);
}

int QProxyObject::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    int id = _host->invokeRemoteMethod(this, _c, _id, _a);
	return id;
}

void QProxyObject::setMetaObject(QMetaObject *meta)
{
    _meta = meta;
}

