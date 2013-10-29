/*void remoteCall(QObject *_o, QMetaObject::Call _c, int _id, void **_arg);

struct RemoteExtraData : public QMetaObjectExtraData
{
	RemoteExtraData(QMetaObjectExtraData *extra, QMetaObject *host)
		: hostInfo(host),
		basicFunc(extra->static_metacall),
		oldExtra(extra)
	{
		objects = extra->objects;
		static_metacall = remoteCall;
	}

	QMetaObjectExtraData *oldExtra;
	QMetaObject *hostInfo;
	StaticMetacallFunction basicFunc;
};

void remoteCall(QObject *_o, QMetaObject::Call _c, int _id, void **_arg)
{
	QMetaObject *meta = const_cast<QMetaObject *>(_o->metaObject());
	RemoteExtraData *extra = 
		reinterpret_cast<RemoteExtraData *>(
		const_cast<void *>(meta->d.extradata));
	qDebug() << "Exploited by occash";
	if(extra) {
		extra->basicFunc(_o, _c, _id, _arg);
	}
}

bool prepareForRemote(QObject *obj)
{
	QMetaObject *meta = const_cast<QMetaObject *>(obj->metaObject());
	QMetaObjectExtraData *extra = 
		reinterpret_cast<QMetaObjectExtraData *>(
		const_cast<void *>(meta->d.extradata));
	//meta->d.extradata = new RemoteExtraData(extra, meta);

	meta->d.superdata = &QObject::staticMetaObject;
	meta->d.data = qt_meta_data;
	meta->d.stringdata = qt_meta_stringdata;
	QMetaObjectExtraData extraData;
	extraData.objects = 0;
	extraData.static_metacall = qt_static_metacall;
	meta->d.extradata = &extraData;

	return true;
}*/

#include <QApplication>

#include "ServerWidget.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	ServertWidget w;
	w.show();

	return a.exec();
}