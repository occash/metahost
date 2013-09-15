#include "qproxyobject.h"

QProxyObject::QProxyObject(QObject *parent) :
    QObject(parent)
{
}


const QMetaObject *QProxyObject::metaObject() const
{
	return _metas.last();
}

void *QProxyObject::qt_metacast(const char *_clname)
{
	if (!_clname) return 0;
	foreach(QMetaObject *meta, _metas)
	{
		if (!strcmp(_clname, meta->d.stringdata))
			return reinterpret_cast<void *>(this);
	}
	return QObject::qt_metacast(_clname);
}

int QProxyObject::qt_metacall(QMetaObject::Call, int, void **)
{
	return 0;
}

