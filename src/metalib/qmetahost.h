#ifndef QMETAHOST_H
#define QMETAHOST_H

#include "global.h"
#include "proto.h"

#include <QObject>
#include <QMap>
#include <QStringList>

class QTcpTransport;
struct QMetaObject;
class QIODevice;

class METAEXPORT QMetaHost : public QObject
{
    Q_OBJECT

public:
    //Move to proto
    enum Command {
        QueryObjectInfo = 0x01,
		ReturnObjectInfo = 0x02,
        QueryClassInfo = 0x03,
		ReturnClassInfo = 0x04,
        CallMetaMethod = 0x05,
		ReturnMetaMethod = 0x06,
		EmitSignal = 0x07
    };
    QMetaHost(QTcpTransport *transport, QObject *parent = 0);
    ~QMetaHost();

    bool registerObject(const QString& name, QObject *);
    QObject *getObject(const QString& name);

private:
	friend class QTcpTransport;
	void processCommand(QIODevice *, QByteArray *data);
    bool checkRevision(const QMetaObject *);
    QString metaName(const QMetaObject *);
    int computeMetaStringSize(const QMetaObject *);
    int computeMetaDataSize(const QMetaObject *);

	static void hostBeginCallback(QObject *caller, int method_index, void **argv);
	static bool initSignalSpy();

private:
    typedef QMap<QString, ObjectMeta> ObjectMap;
    typedef QMap<QString, ClassMeta> ClassMap;
    ObjectMap _objects;
    ClassMap _classes;
    QTcpTransport *_transport;

	//static QList<QMetaHost *> _hosts;
	static QMetaHost * _hosts;
	static bool _initSpy;

signals:
	void gotObjectInfo();
	void gotClassInfo();

};

#endif // QMETAHOST_H
