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
    QProxyObject(QObject *parent = 0);
    void setMetaObject(QMetaObject *meta);

private:
	QMetaObject *_meta;
    
};

#endif // QPROXYOBJECT_H
