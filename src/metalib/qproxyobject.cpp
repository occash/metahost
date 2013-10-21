#include "qproxyobject.h"
#include "qmetahost.h"

#include <QDebug>
#include <QMetaObject>

QProxyObject::QProxyObject(QMetaObject *meta, QObject *parent) :
    QObject(parent),
    _meta(meta)
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
	qDebug() << _c << _id;
	return _id;
}

