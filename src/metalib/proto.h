#ifndef PROTO_H
#define PROTO_H

#include <QObject>
#include <QMetaObject>
#include <QString>
#include <QStringList>
#include <QDataStream>

struct ClassMeta
{
	QMetaObject *metaObject;
	quint32 dataSize;
	quint32 stringSize;
};

QDataStream &operator<<(QDataStream &out, const ClassMeta &classMeta);
QDataStream &operator>>(QDataStream &in, ClassMeta &classMeta);

struct ObjectMeta
{
	bool fullQuilified;
	QObject *object;
	QStringList classInfo;
	bool remote;
};

QDataStream &operator<<(QDataStream &out, const ObjectMeta &objMeta);
QDataStream &operator>>(QDataStream &in, ObjectMeta &objMeta);

#endif // PROTO_H
