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
    QObject *getObject(const QString& name, QObject *client);

protected:
    bool event(QEvent *e);

private:
	void processCommand(QObject *client, char *data);
    bool checkRevision(const QMetaObject *);
    QString metaName(const QMetaObject *);
    quint16 computeMetaStringSize(const QMetaObject *);
    quint16 computeMetaDataSize(const QMetaObject *);
    QObject *findObjectByName(const QString& name);

    static void hostCallback(QObject *caller, int method_index, void **argv);
    static void signalCallback(QObject *caller, int method_index, void **argv);
    static void slotCallback(QObject *caller, int method_index, void **argv);
	static bool initSignalSpy();

private:
    typedef QMap<QObject *, QObjectList *> LocalObjectMap;
    typedef QMap<QObject *, ObjectMeta> RemoteObjectMap;
    typedef QMap<quint32, QObject *> RemoteHelperMap;
    typedef QMap<QString, ClassMeta> ClassMap;

    LocalObjectMap _localObjects;
    RemoteObjectMap _remoteObjects;
    RemoteHelperMap _remoteIds;
    ClassMap _classes;

	static QMetaHost * _host;
	static bool _initSpy;

signals:
	void gotObjectInfo();
	void gotClassInfo();

private:
    void prepareQuery(char *data, char **answer, quint8 ctype);
    void makeQuery(QObject *client, const QString& name, 
        quint8 ctype, const char *signal);
    bool constructObject(QObject *object, const QStringList& classes);
    //Processing stuff
    void processQueryObjectInfo(QObject *client, char *data, char **answer);
    void processReturnObjectInfo(QObject *client, char *data, char **answer);
    void processQueryClassInfo(QObject *client, char *data, char **answer);
    void processReturnClassInfo(QObject *client, char *data, char **answer);
    void processEmitSignal(QObject *client, char *data, char **answer);

};

#endif // QMETAHOST_H
