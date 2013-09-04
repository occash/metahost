#ifndef QMETAHOST_H
#define QMETAHOST_H

#include "global.h"

#include <QObject>
#include <QMap>
#include <QStringList>

class QTcpTransport;
struct QMetaObject;
class QIODevice;

// ************Move to proto****************************************
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
    //Move to proto
    enum Command {
        ObjectInfo = 0x01,
        ClassInfo = 0x02,
        MethodCall = 0x03
    };
    QMetaHost(QTcpTransport *transport, QObject *parent = 0);
    ~QMetaHost() {}

    bool registerObject(const QString& name, QObject *);
    void processCommand(QIODevice *, QByteArray *data);

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
    QTcpTransport *_transport;

};

#endif // QMETAHOST_H
