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

#ifndef QPROXYOBJECT_H
#define QPROXYOBJECT_H

#include <QObject>

#include "proto.h"

class QMetaHost;
struct QMetaObject;

class QProxyObject : public QObject
{
public:
    Q_OBJECT_CHECK
    QT_TR_FUNCTIONS

    virtual const QMetaObject *metaObject() const;
    virtual void *qt_metacast(const char *);
    virtual int qt_metacall(QMetaObject::Call, int, void **);

private:
	friend class QMetaHost;
    QProxyObject(QMetaHost *host, QObject *parent = 0);
    void setMetaObject(QMetaObject *meta);

private:
    QMetaHost *_host;
	QMetaObject *_meta;

    struct {
        QMetaObject::Call callType;
        int methodIndex;
        int returnId;
        void *returnArg;
    } ret;
    
};

#endif // QPROXYOBJECT_H
