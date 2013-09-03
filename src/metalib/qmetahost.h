#ifndef QMETAHOST_H
#define QMETAHOST_H

#include "global.h"

#include <QObject>
#include <QMap>

class QTcpTransport;
struct QMetaObject;

class METAEXPORT QMetaHost : public QObject
{
    Q_OBJECT

public:
    QMetaHost(QTcpTransport *transport, QObject *parent = 0);
    ~QMetaHost() {}

    bool registerObject(QObject *);

private:
    bool checkRevision(const QMetaObject *);

private:
    typedef QMap<QString, QObject *> ObjectMap;
    ObjectMap _objects;

};

#endif // QMETAHOST_H
