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
	void processCommand(char *data, int size);
    bool checkRevision(const QMetaObject *);
    QString metaName(const QMetaObject *);
    int computeMetaStringSize(const QMetaObject *);
    int computeMetaDataSize(const QMetaObject *);

	static void hostBeginCallback(QObject *caller, int method_index, void **argv);
	static bool initSignalSpy();

private:
    typedef QMap<quint32, QObject *> LocalObjectMap;
    typedef QMap<QObject *, ObjectMeta> RemoteObjectMap;
    typedef QMap<QString, ClassMeta> ClassMap;
    LocalObjectMap _localObjects;
    RemoteObjectMap _objects;
    ClassMap _classes;

	//static QList<QMetaHost *> _hosts;
	static QMetaHost * _hosts;
	static bool _initSpy;

signals:
	void gotObjectInfo();
	void gotClassInfo();

private:
    void processQueryObjectInfo(char *data, char **answer, int *answerSize);
    void processReturnObjectInfo(char *data);
};

#endif // QMETAHOST_H
