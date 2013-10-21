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
    QMetaHost(QObject *parent = 0);
    ~QMetaHost();

    bool registerObject(QObject *);
    QObject *getObject(const QString& name);

protected:
    bool event(QEvent *e);

private:
	void processCommand(char *data);
    bool checkRevision(const QMetaObject *);
    QString metaName(const QMetaObject *);
    quint16 computeMetaStringSize(const QMetaObject *);
    quint16 computeMetaDataSize(const QMetaObject *);

	static void hostBeginCallback(QObject *caller, int method_index, void **argv);
	static bool initSignalSpy();

private:
    typedef QMap<quint32, QObject *> LocalObjectMap;
    typedef QMap<QObject *, ObjectMeta> RemoteObjectMap;
    typedef QMap<QString, ClassMeta> ClassMap;
    LocalObjectMap _localObjects;
    RemoteObjectMap _objects;
    ClassMap _classes;

	static QMetaHost * _host;
	static bool _initSpy;

signals:
	void gotObjectInfo(ObjectMeta);
	void gotClassInfo(ClassMeta);

private:
    void processQueryObjectInfo(char *data, char **answer);
    void processReturnObjectInfo(char *data, char **answer);
    void processQueryClassInfo(char *data, char **answer);
    void processReturnClassInfo(char *data, char **answer);
};

#endif // QMETAHOST_H
