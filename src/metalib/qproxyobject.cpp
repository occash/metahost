#include "qproxyobject.h"
#include "qmetahost.h"

QProxyObject::QProxyObject(const QList<ClassMeta>& metas, QObject *parent) :
    QObject(parent),
	_metas(metas)
{
}

const QMetaObject *QProxyObject::metaObject() const
{
	return _metas.first().metaObject;
}

void *QProxyObject::qt_metacast(const char *_clname)
{
	if (!_clname) return 0;
	foreach(const ClassMeta& meta, _metas)
	{
		if (!strcmp(_clname, meta.metaObject->d.stringdata))
			return reinterpret_cast<void *>(this);
	}
	return QObject::qt_metacast(_clname);
}

int QProxyObject::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
	qDebug() << _c << _id;
	return _id;
}

