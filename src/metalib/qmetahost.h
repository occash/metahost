/*********************************************************************
This file is part of the MetaHost library.
Copyright (C) 2013 Artem Shal
artiom.shal@gmail.com

The MetaHost library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

This software is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
USA.
**********************************************************************/

#ifndef QMETAHOST_H
#define QMETAHOST_H

#include "global.h"
#include "proto.h"
#include "paramholder.h"

#include <QObject>
#include <QMap>
#include <QStringList>

class QTcpTransport;
struct QMetaObject;
class QIODevice;
class QProxyObject;

class METAEXPORT QMetaHost : public QObject
{
    Q_OBJECT

public:
    QMetaHost(QObject *parent = 0);
    ~QMetaHost();

    template<class T>
    bool registerClass();
    bool registerObject(QObject *);
    QObject *getObject(const QString& name, QObject *client);

protected:
    bool event(QEvent *e);

private slots:
    void onDestroyed(QObject *);

private:
    //Processing stuff
	void processCommand(QObject *client, char *data);
    void processQueryObjectInfo(QObject *client, char *data, char **answer);
    void processReturnObjectInfo(QObject *client, char *data, char **answer);
    void processQueryClassInfo(QObject *client, char *data, char **answer);
    void processReturnClassInfo(QObject *client, char *data, char **answer);
    void processEmitSignal(QObject *client, char *data, char **answer);
    void processCallMetaMethod(QObject *client, char *data, char **answer);
    void processReturnMetaMethod(QObject *client, char *data, char **answer);

    //Helper functions
    bool registerMetaClass(const QMetaObject *meta);
    bool checkRevision(const QMetaObject *);
    QString metaName(const QMetaObject *);
    quint16 computeMetaStringSize(const QMetaObject *);
    quint16 computeMetaDataSize(const QMetaObject *);
    QObject *findObjectByName(const QString& name);
    void prepareQuery(char *data, char **answer, quint8 ctype);
    void makeQuery(QObject *client, const QString& name, 
        quint8 ctype, const char *signal);
    bool constructObject(QObject *object, const QStringList& classes);
    void prepareReturnMetaMethod(quint32 o, QMetaObject::Call c, int id, 
        void **argv, int typeId, int ret, char **answer);

    //Callback functions
    static void hostCallback(QObject *caller, int method_index, void **argv);
	static bool initSignalSpy();

    friend class QProxyObject;
    int invokeRemoteMethod(QObject *_o, QMetaObject::Call _c, int _id, void **_a);

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
    void gotMethodReturn(ReturnParam param);

};

template<class T>
bool QMetaHost::registerClass()
{
    const QMetaObject *meta = &T::staticMetaObject;
    return registerMetaClass(meta);
}

#endif // QMETAHOST_H
