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

#ifndef PROTO_H
#define PROTO_H

#include <QObject>
#include <QMetaObject>
#include <QStringList>

struct ClassMeta
{
    QMetaObject *metaObject;
    quint16 dataSize;
    quint16 stringSize;
};

struct ObjectMeta
{
    quint32 id;
    QObject *client;
	bool qualified;
    QStringList classes;
};

#define QueryObjectInfo 0x01
#define ReturnObjectInfo 0x02
#define QueryClassInfo 0x03
#define ReturnClassInfo 0x04
#define CallMetaMethod 0x05
#define ReturnMetaMethod 0x06
#define EmitSignal 0x07

#endif // PROTO_H
