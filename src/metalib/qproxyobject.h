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
