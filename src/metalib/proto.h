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
