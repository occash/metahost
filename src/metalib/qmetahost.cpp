#include "qmetahost.h"
#include "qtcptransport.h"

#include <QMetaObject>
#include <QDebug>

QMetaHost::QMetaHost(QTcpTransport *transport, QObject *parent)
    : QObject(parent)
{
    transport->setParent(this);
    connect(transport, SIGNAL(newClient(QIODevice*)), this, SLOT(onNewClient(QIODevice*)));
    connect(transport, SIGNAL(removeClient(QIODevice*)), this, SLOT(onRemoveClient(QIODevice*)));
}


bool QMetaHost::registerObject(QObject *object)
{
    bool result = true;
    const QMetaObject *meta = object->metaObject();
    if(!meta)
        return false;

    result &= checkRevision(meta);

    return result;
}

bool QMetaHost::checkRevision(const QMetaObject *meta)
{
    const uint *mdata = meta->d.data;
    quint32 revision = mdata[0];
    if(revision != 6) {
        qDebug() << "Error: QMetaHost only compatible with revision 6.";
        return false;
    }

    return true;
}
