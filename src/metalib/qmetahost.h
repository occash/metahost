#ifndef QMETAHOST_H
#define QMETAHOST_H

#include "global.h"

#include <QObject>
#include <QMap>
#include <QStringList>

class QTcpTransport;
struct QMetaObject;

struct ClassMeta
{
    QMetaObject *metaObject;
    quint32 dataSize;
    quint32 stringSize;
};

struct ObjectMeta
{
    QObject *object;
    QStringList classInfo;
};

class METAEXPORT QMetaHost : public QObject
{
    Q_OBJECT

public:
    QMetaHost(QTcpTransport *transport, QObject *parent = 0);
    ~QMetaHost() {}

    bool registerObject(const QString& name, QObject *);

private:
    bool checkRevision(const QMetaObject *);
    QString metaName(const QMetaObject *);
    int computeMetaStringSize(const QMetaObject *);
    int computeMetaDataSize(const QMetaObject *);

private:
    typedef QMap<QString, ObjectMeta> ObjectMap;
    typedef QMap<QString, ClassMeta> ClassMap;
    ObjectMap _objects;
    ClassMap _classes;

};

#endif // QMETAHOST_H
