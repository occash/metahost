#include "qproxyobject.h"
#include "qmetahost.h"

#include <QDebug>

QProxyObject::QProxyObject(const QList<ClassMeta>& metas, QObject *parent) :
    QObject(parent)
{
	foreach(const ClassMeta& meta, metas)
	{
		_metas.append(meta.metaObject);
		if(_metas.size() > 1)
		{
			QMetaObject *lastObj = _metas.at(_metas.size() - 1);
			QMetaObject *prevObj = _metas.at(_metas.size() - 2);
			prevObj->d.superdata = lastObj;
			prevObj->d.extradata = nullptr;
		}
	}
	QMetaObject *lastObj = _metas.at(_metas.size() - 1);
	lastObj->d.superdata = nullptr;
}

const QMetaObject *QProxyObject::metaObject() const
{
	return _metas.first();
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

int QProxyObject::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
	qDebug() << _c << _id;
	return _id;
}

