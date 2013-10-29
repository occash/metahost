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

#include "qmetahost.h"
#include "qtcptransport.h"
#include "qproxyobject.h"
#include "qmetaevent.h"

#include <QMetaObject>
#include <QDebug>
#include <QDataStream>
#include <QIODevice>
#include <QMetaMethod>
#include <QEventLoop>
#include <QTimer>
#include <QMetaObject>
#include <QCoreApplication>
#include <QObjectList>

bool prepareRawParams(QByteArray *ret, const QMetaObject *meta, int method_index, void **argv)
{
    const QMetaObject *metaObject = meta;
    QMetaMethod metaMethod = metaObject->method(method_index);
    QList<QByteArray> paramTypes = metaMethod.parameterTypes();
    QByteArray argData;
    QDataStream argStream(&argData, QIODevice::WriteOnly);

    for(int i = 0; i < paramTypes.size(); ++i)
    {
        const QByteArray& typeName = paramTypes.at(i);
        int typeId = QMetaType::type(typeName.constData());
        bool saved = QMetaType::save(argStream, typeId, argv[i + 1]);
        if(!saved)
        {
            qWarning() << "Cannot read meta type" << typeName;
            return false;
        }
    }

    *ret = argData;

    return true;
}

//******************************Private hooks************************************

struct QSignalSpyCallbackSet
{
	typedef void (*BeginCallback)(QObject *caller, int method_index, void **argv);
	typedef void (*EndCallback)(QObject *caller, int method_index);
	BeginCallback signal_begin_callback,
		slot_begin_callback;
	EndCallback signal_end_callback,
		slot_end_callback;
};

extern void Q_CORE_EXPORT qt_register_signal_spy_callbacks(const QSignalSpyCallbackSet &);

#include <iostream>

void QMetaHost::hostCallback(QObject *caller, int method_index, void **argv)
{
    QMetaHost *host = nullptr;
	if(!(host = _host))
		return;

    auto i = host->_localObjects.find(caller);
    if(i == host->_localObjects.end())
        return;

    int headerSize = sizeof(quint16);
    quint16 answerSize = sizeof(quint8) + sizeof(quint32) + 
        sizeof(int) + sizeof(quint16);

    QByteArray argData;
    const QMetaObject *metaObject = caller->metaObject();
	if(!prepareRawParams(&argData, metaObject, method_index, argv))
        return;

    answerSize += argData.size();
    //Allocate memory for answer
    char *msg = new char[headerSize + answerSize];
    char *ptr = msg;

    //Set header size
    *((quint16 *)ptr) = answerSize;
    ptr += sizeof(quint16);

    //Set header type
    quint8 type = EmitSignal;
    *((quint8 *)ptr) = type;
    ptr += sizeof(quint8);

    //Set generated object id
    *((quint32 *)ptr) = reinterpret_cast<quint32>(caller);
    ptr += sizeof(quint32);
	
    //Set method index
    *((int *)ptr) = method_index;
    ptr += sizeof(int);

    //Check size on remote side!
    *((quint16 *)ptr) = 0;

    if(argData.size() > 0)
    {
        //Set raw data size
        *((quint16 *)ptr) = argData.size();
        ptr += sizeof(quint16);

        //Copy raw parameters data
        memcpy(ptr, argData.data(), argData.size());
    }

    foreach(QObject *client, *(*i))
    {
        QMetaEvent *me = new QMetaEvent(client, msg);
        QCoreApplication::postEvent(client, me, Qt::HighEventPriority);
    }
}

bool QMetaHost::initSignalSpy()
{
	QSignalSpyCallbackSet callback;
	callback.signal_begin_callback = QMetaHost::hostCallback;
	callback.signal_end_callback = nullptr;
	callback.slot_begin_callback = nullptr;
	callback.slot_end_callback = nullptr;
	qt_register_signal_spy_callbacks(callback);
	return true;
}

QMetaHost * QMetaHost::_host = nullptr;
bool QMetaHost::_initSpy = QMetaHost::initSignalSpy();

int QMetaHost::invokeRemoteMethod(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto o = _remoteObjects.find(_o);
    if(o == _remoteObjects.end())
        return 1;

    QProxyObject *proxy = qobject_cast<QProxyObject *>(o.key());
    if(!proxy)
        return 1;

    proxy->ret.returnArg = _a[0];

    int answerSize = sizeof(quint8) + sizeof(quint32) + 
        sizeof(QMetaObject::Call) + sizeof(int) + sizeof(quint16);

    QByteArray argData;
    const QMetaObject *metaObject = _o->metaObject();
    if(!prepareRawParams(&argData, metaObject, _id, _a))
        return 1;

    answerSize += argData.size();
    //Allocate memory for answer
    int headerSize = sizeof(quint16);
    char *msg = new char[headerSize + answerSize];
    char *ptr = msg;

    //Set header size
    *((quint16 *)ptr) = answerSize;
    ptr += sizeof(quint16);

    //Set header type
    quint8 type = CallMetaMethod;
    *((quint8 *)ptr) = type;
    ptr += sizeof(quint8);

    //Set qt_meta_call raw params
    quint32 objectId = (*o).id;
    *((quint32 *)ptr) = objectId;
    ptr += sizeof(quint32);

    *((QMetaObject::Call *)ptr) = _c;
    ptr += sizeof(QMetaObject::Call);

    *((int *)ptr) = _id;
    ptr += sizeof(int);

    //Check size on remote side!!
    *((quint16 *)ptr) = 0;

    if(argData.size() > 0)
    {
        //Set raw data size
        *((quint16 *)ptr) = argData.size();
        ptr += sizeof(quint16);

        //Copy raw parameters data
        memcpy(ptr, argData.data(), argData.size());
    }

    QMetaEvent *me = new QMetaEvent((*o).client, msg);
    QCoreApplication::postEvent((*o).client, me, Qt::HighEventPriority);

    QEventLoop loop;
    QTimer timer;
    connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
    connect(this, SIGNAL(gotMethodReturn()), &loop, SLOT(quit()));
    timer.start(3000);
    loop.exec();

    if(proxy->ret.callType != _c ||
        proxy->ret.methodIndex != _id)
        return 1;

    return proxy->ret.returnId;
}

//**************************************Meta host*********************************

QMetaHost::QMetaHost(QObject *parent)
    : QObject(parent)
{
	_host = this;
}

QMetaHost::~QMetaHost()
{
    auto i = _localObjects.begin();
    while(i != _localObjects.end())
    {
        delete (*i);
        i = _localObjects.erase(i);
    }
}

bool QMetaHost::registerObject(QObject *object)
{
    const QMetaObject *meta = object->metaObject();
    if(!meta)
        return false;

    bool result = registerMetaClass(meta);

    if(result)
        _localObjects.insert(object, new QObjectList());

    return result;
}

//Processing stuff

void QMetaHost::processQueryObjectInfo(QObject *client, char *data, char **answer)
{
    QString objectName(data);
    qDebug() << "Query object info:" << objectName;

    //Locate the desired objects
    QObject *object = nullptr;
    quint32 id = 0;
    auto i = _localObjects.begin();
    for(; i != _localObjects.end(); ++i)
    {
        if(i.key()->objectName() == objectName) {
            id = reinterpret_cast<quint32>(i.key());
            object = i.key();
            if((*i)->indexOf(client) == -1)
                (*i)->append(client);

            if(!_clients.contains(client))
            {
                _clients.append(client);
                connect(client, SIGNAL(destroyed(QObject *)), 
                    this, SLOT(onDestroyed(QObject *)));
            }

            break;
        }
    }

    //Compute size of packet required
    int answerSize = sizeof(quint8) + sizeof(quint32) + objectName.size() + 2; 
    if(object)
    {
        QMetaObject *meta = const_cast<QMetaObject *>(
            object->metaObject());
        while(meta)
        {
            answerSize += strlen(meta->d.stringdata) + 1;
            meta = const_cast<QMetaObject *>(meta->d.superdata);
        }
    }

    //Allocate memory for answer
    int headerSize = sizeof(quint16);
    *answer = new char[headerSize + answerSize];
    char *ptr = *answer;

    //Set header size
    *((quint16 *)ptr) = answerSize;
    ptr += sizeof(quint16);

    //Set header type
    quint8 type = ReturnObjectInfo;
    *((quint8 *)ptr) = type;
    ptr += sizeof(quint8);

    //Set name of object
    memcpy(ptr, data, objectName.size() + 1);
    ptr += objectName.size() + 1;

    //Set generated object id
    *((quint32 *)ptr) = id;
    ptr += sizeof(quint32);

    //Set class names related to object
    if(object)
    {
        QMetaObject *meta = const_cast<QMetaObject *>(
            object->metaObject());
        while(meta)
        {
            int len = strlen(meta->d.stringdata) + 1;
            memcpy(ptr, meta->d.stringdata, len);
            ptr += len;
            meta = const_cast<QMetaObject *>(meta->d.superdata);
        }
    }

    //Add terminating zero
    *ptr = '\0';
}

void QMetaHost::processReturnObjectInfo(QObject *client, char *data, char **answer)
{
    QString objectName(data);
    char *ptr = data;
    ptr += objectName.size() + 1;
    *answer = 0;
    quint32 id = *((quint32 *)ptr);
    //Remote side didn't found object
    if(!id)
        return;

    //Construct empty ObjectMeta
    ObjectMeta objectMeta;
    objectMeta.id = id;
    objectMeta.qualified = true;
    objectMeta.client = client;
    ptr += sizeof(quint32);

    //Traverse through class names required to build object
    //and decide if object is fully qualified
    while(*ptr)
    {
        QString className(ptr);

        auto i = _classes.find(className);
        if(i == _classes.end())
            objectMeta.qualified &= false;

        objectMeta.classes << className;
        ptr += className.size() + 1;
    }

    QProxyObject *proxy = new QProxyObject(this);
    proxy->setObjectName(objectName);
    _remoteObjects.insert(proxy, objectMeta);
    _remoteIds.insert(id, proxy);

    //Construct object if we have enough information
    if(objectMeta.qualified)
    {
        constructObject(proxy, objectMeta.classes);
    }

    emit gotObjectInfo();
}

void QMetaHost::processQueryClassInfo(QObject *client, char *data, char **answer)
{
    QString className(data);
    qDebug() << "Query class info:" << className;

    //Compute size of packet required
    int answerSize = sizeof(quint8) + 1;
    answerSize += className.size() + 1;

    auto i = _classes.find(className);
    if(i != _classes.end())
    {        
        answerSize += sizeof(quint16) * 2;
        answerSize += (*i).dataSize * sizeof(uint);
        answerSize += (*i).stringSize;
    }

    //Allocate memory for answer
    int headerSize = sizeof(quint16);
    *answer = new char[headerSize + answerSize];
    char *ptr = *answer;

    //Set header size
    *((quint16 *)ptr) = answerSize;
    ptr += sizeof(quint16);

    //Set header type
    quint8 type = ReturnClassInfo;
    *((quint8 *)ptr) = type;
    ptr += sizeof(quint8);

    memcpy(ptr, data, className.size() + 1);
    ptr += className.size() + 1;

    *((bool *)ptr) = false; //Found flag (false by default)

    //Copy meta information to message
    if(i != _classes.end())
    {
        *((bool *)ptr) = true;
        ptr += sizeof(bool);

        *((quint16 *)ptr) = (*i).dataSize;
        ptr += sizeof(quint16);

        memcpy(ptr, (*i).metaObject->d.data, (*i).dataSize * sizeof(uint));
        ptr += (*i).dataSize * sizeof(uint);

        *((quint16 *)ptr) = (*i).stringSize;
        ptr += sizeof(quint16);

        memcpy(ptr, (*i).metaObject->d.stringdata, (*i).stringSize);
        ptr += (*i).stringSize;
    }
}

void QMetaHost::processReturnClassInfo(QObject *client, char *data, char **answer)
{
    QString className(data);
    data += className.size() + 1;

    bool found = *((bool *)data);
    //Remote side didn't found class definition
    if(!found)
        return;

    data += sizeof(bool);
    ClassMeta classMeta;
    QMetaObject *classInfo = new QMetaObject();
    
    quint16 dataSize = *((quint16 *)data);
    classMeta.dataSize = dataSize;
    data += sizeof(quint16);
    classInfo->d.data = new uint[dataSize];
    memcpy((void *)classInfo->d.data, data, dataSize * sizeof(uint));
    data += dataSize * sizeof(uint);

    quint16 stringSize = *((quint16 *)data);
    classMeta.stringSize = stringSize;
    data += sizeof(quint16);
    classInfo->d.stringdata = new char[stringSize];
    memcpy((void *)classInfo->d.stringdata, data, stringSize);
    data += stringSize;

    classMeta.metaObject = classInfo;
    _classes.insert(className, classMeta);

    emit gotClassInfo();
}

void QMetaHost::processEmitSignal(QObject *client, char *data, char **answer)
{
    quint32 id = *((quint32 *)data);
    data += sizeof(quint32);

    auto o = _remoteIds.find(id);
    if(o == _remoteIds.end())
        return;

    int method_index = *((int *)data);
    data += sizeof(quint32);

    const QMetaObject *metaObject = (*o)->metaObject();
    while (metaObject) {
        if(method_index >= metaObject->methodOffset())
            break;

        metaObject = metaObject->d.superdata;
    }

    quint32 local_index = method_index - metaObject->methodOffset();
    QMetaMethod metaMethod = metaObject->method(method_index);
    QList<QByteArray> paramTypes = metaMethod.parameterTypes();
    void **argv = 0;

    //Allocate memory for raw params
    int arrayLen = paramTypes.size() + 1;
    argv = (void **)qMalloc(arrayLen * sizeof(void *));

    const char *retName = metaMethod.typeName();
    if(retName)
    {
        int typeId = QMetaType::type(retName);
        argv[0] = QMetaType::construct(typeId);
    }
    else
    {
        argv[0] = 0;
    }

    if(paramTypes.size() > 0)
    {
        quint16 dataSize = *((quint16 *)data);
        data += sizeof(quint16);        

        QByteArray argData = QByteArray::fromRawData(data, dataSize);
        QDataStream argStream(&argData, QIODevice::ReadOnly);

        for(int i = 0; i < paramTypes.size(); ++i)
        {
            const QByteArray& typeName = paramTypes.at(i);
            int typeId = QMetaType::type(typeName.constData());
            argv[i + 1] = QMetaType::construct(typeId);
            bool loaded = QMetaType::load(argStream, typeId, argv[i + 1]);
            if(!loaded)
            {
                qWarning() << "Cannot load meta type" << typeName;
                return;
            }
        }
    }
    
	QMetaObject::activate((*o), metaObject, local_index, argv);
}

void QMetaHost::processCallMetaMethod(QObject *client, char *data, char **answer)
{
    //First check if object is registered
    //_o, _c, _id, _a
    int retId = 1;
    QObject *object = *((QObject **)data);
    data += sizeof(QObject *);

    auto o = _localObjects.find(object);
    if(o == _localObjects.end())
        return; //Send no such method

    QMetaObject::Call c = *((QMetaObject::Call *)data);
    data += sizeof(QMetaObject::Call);

    int id = *((int *)data);
    data += sizeof(int);

    //TODO: remove code duplication
    const QMetaObject *metaObject = object->metaObject();
    QMetaMethod metaMethod = metaObject->method(id);
    QList<QByteArray> paramTypes = metaMethod.parameterTypes();
    void **argv = 0;

    //Allocate memory for raw params
    int arrayLen = paramTypes.size() + 1;
    argv = (void **)qMalloc(arrayLen * sizeof(void *));

    const char *retName = metaMethod.typeName();
    int retTypeId = -1;
    if(retName)
    {
        retTypeId = QMetaType::type(retName);
        argv[0] = QMetaType::construct(retTypeId);
    }
    else
    {
        argv[0] = 0;
    }

    if(paramTypes.size() > 0)
    {
        quint16 dataSize = *((quint16 *)data);
        data += sizeof(quint16);
        
        QByteArray argData = QByteArray::fromRawData(data, dataSize);
        QDataStream argStream(&argData, QIODevice::ReadOnly);

        for(int i = 0; i < paramTypes.size(); ++i)
        {
            const QByteArray& typeName = paramTypes.at(i);
            int typeId = QMetaType::type(typeName.constData());
            argv[i + 1] = QMetaType::construct(typeId);
            bool loaded = QMetaType::load(argStream, typeId, argv[i + 1]);
            if(!loaded)
            {
                qWarning() << "Cannot load meta type" << typeName;
                return; //Send no such method
            }
        }
    }

    retId = QMetaObject::metacall(object, c, id, argv);

    int answerSize = sizeof(quint8) + sizeof(quint32) + 
        sizeof(QMetaObject::Call) + sizeof(int) * 2 + sizeof(quint16);

    QByteArray argData;
    if(retName)
    {
        QDataStream argStream(&argData, QIODevice::WriteOnly);
        bool saved = QMetaType::save(argStream, retTypeId, argv[0]);
        if(!saved)
            return; //Send no such method
    }

    answerSize += argData.size();
    //Allocate memory for answer
    int headerSize = sizeof(quint16);
    *answer = new char[headerSize + answerSize];
    char *ptr = *answer;

    //Set header size
    *((quint16 *)ptr) = answerSize;
    ptr += sizeof(quint16);

    //Set header type
    quint8 type = ReturnMetaMethod;
    *((quint8 *)ptr) = type;
    ptr += sizeof(quint8);

    //Set qt_meta_call raw params
    quint32 objectId = reinterpret_cast<quint32>(object);
    *((quint32 *)ptr) = objectId;
    ptr += sizeof(quint32);

    *((QMetaObject::Call *)ptr) = c;
    ptr += sizeof(QMetaObject::Call);

    *((int *)ptr) = id;
    ptr += sizeof(int);

    *((int *)ptr) = retId;
    ptr += sizeof(int);

    //Check size on remote side!!
    *((quint16 *)ptr) = 0;

    if(retName && argData.size() > 0)
    {
        //Set raw data size
        *((quint16 *)ptr) = argData.size();
        ptr += sizeof(quint16);

        //Copy raw parameters data
        memcpy(ptr, argData.data(), argData.size());
    }
}

void QMetaHost::processReturnMetaMethod(QObject *client, char *data, char **answer)
{
    quint32 objectId = *((quint32 *)data);
    data += sizeof(QObject *);

    auto o = _remoteIds.find(objectId);
    if(o == _remoteIds.end())
        return;

    QMetaObject::Call c = *((QMetaObject::Call *)data);
    data += sizeof(QMetaObject::Call);

    int id = *((int *)data);
    data += sizeof(int);

    int retId = *((int *)data);
    data += sizeof(int);
    if(retId > 0)
        return; //Die fast

    const QMetaObject *metaObject = (*o)->metaObject();
    QMetaMethod metaMethod = metaObject->method(id);
    const char *retName = metaMethod.typeName();

    QProxyObject *proxy = qobject_cast<QProxyObject *>(*o);
    if(!proxy)
        return;

    if(retName)
    {
        quint16 dataSize = *((quint16 *)data);
        data += sizeof(quint16);

        QByteArray argData = QByteArray::fromRawData(data, dataSize);
        QDataStream argStream(&argData, QIODevice::ReadOnly);

        int retTypeId = QMetaType::type(retName);
        bool loaded = QMetaType::load(argStream, retTypeId, proxy->ret.returnArg);
        if(!loaded)
        {
            qWarning() << "Cannot load meta type" << retName;
            return;
        }
    }

    proxy->ret.callType = c;
    proxy->ret.methodIndex = id;
    proxy->ret.returnId = retId;

    emit gotMethodReturn();
}

void QMetaHost::processCommand(QObject *client, char *data)
{
    char *ptr = data;
    quint8 command = *((quint8 *)data);
    data += sizeof(quint8);
    char *answer = nullptr;

    switch(command)
    {
    case QueryObjectInfo:
		processQueryObjectInfo(client, data, &answer);
        break;
	case ReturnObjectInfo:
        processReturnObjectInfo(client, data, &answer);
		break;
    case QueryClassInfo:
		processQueryClassInfo(client, data, &answer);
        break;
	case ReturnClassInfo:
		processReturnClassInfo(client, data, &answer);
		break;
    case CallMetaMethod:
        processCallMetaMethod(client, data, &answer);
        break;
    case ReturnMetaMethod:
        processReturnMetaMethod(client, data, &answer);
        break;
	case EmitSignal:
		processEmitSignal(client, data, &answer);
		break;
    default:
        break;
    }

    delete ptr;
    if(answer)
    {
        QMetaEvent *me = new QMetaEvent(client, answer);
        QCoreApplication::postEvent(client, me, Qt::HighEventPriority);
    }
}

bool QMetaHost::event(QEvent *e)
{
    if(e->type() == MetaEventType)
    {
        QMetaEvent *me = dynamic_cast<QMetaEvent *>(e);
        if(me)
            processCommand(me->container(), me->data());

        return true;
    }
    return false;
}

bool QMetaHost::checkRevision(const QMetaObject *meta)
{
    const uint *mdata = meta->d.data;
    quint32 revision = mdata[0];
    if(revision != 6) {
        qDebug() << "Error: QMetaHost only compatible with revision 6.";
        return false;
    }

    return true;
}

QString QMetaHost::metaName(const QMetaObject *meta)
{
    const uint *mdata = meta->d.data;
    const char *msdata = meta->d.stringdata;

    QString metaName = msdata + mdata[1];

    return metaName;
}

quint16 QMetaHost::computeMetaStringSize(const QMetaObject *meta)
{
    const uint *mdata = meta->d.data;
    if(!mdata)
        return -1;

    char *msdata = const_cast<char *>(meta->d.stringdata);
    if(!msdata)
        return -1;

    char *lastChar = msdata;

    {//Class name
        uint classShift = mdata[1];
        if(msdata + classShift > lastChar)
            lastChar = msdata + classShift;
    }

    {//Class info
        uint cinum = mdata[2];
        uint cishift = mdata[3];
        for(uint i = 0; i < cinum; ++i) {
            uint cikey = mdata[cishift + i];
            uint cival = mdata[cishift + i + 1];
            if(msdata + cikey > lastChar)
                lastChar = msdata + cikey;
            if(msdata + cival > lastChar)
                lastChar = msdata + cival;
        }
    }

    {//Class methods
        uint cmnum = mdata[4];
        uint cmshift = mdata[5];
        for(uint i = 0; i < cmnum; ++i) {
            for(uint j = 0; j < 4; ++j) {
                char *cmsdata = msdata + mdata[cmshift + i * 5 + j];
                if(cmsdata > lastChar)
                    lastChar = cmsdata;
            }
        }
    }

    {//Class properties
        uint cpnum = mdata[6];
        uint cpshift = mdata[7];
        for(uint i = 0; i < cpnum; ++i) {
            for(uint j = 0; j < 2; ++j) {
                char *cpsdata = msdata + mdata[cpshift + i * 3 + j];
                if(cpsdata > lastChar)
                    lastChar = cpsdata;
            }
        }
    }

    {//Class enums/sets
        uint cenum = mdata[8];
        uint ceshift = mdata[9];
        for(uint i = 0; i < cenum; ++i) {
            char *cename = msdata + mdata[ceshift + i * 4];
            if(cename > lastChar)
                lastChar = cename;

            uint cecount = mdata[ceshift + i * 4 + 2];
            uint cedata = mdata[ceshift + i * 4 + 3];
            for(uint j = 0; j < cecount; ++j) {
                char *ceval = msdata + mdata[cedata + j * 2];
                if(ceval > lastChar)
                    lastChar = ceval;
            }
        }
    }

    {//Class constructors
        uint ccnum = mdata[10];
        uint ccshift = mdata[11];
        for(uint i = 0; i < ccnum; ++i) {
            for(uint j = 0; j < 4; ++j) {
                char *ccsdata = msdata + mdata[ccshift + i * 5 + j];
                if(ccsdata > lastChar)
                    lastChar = ccsdata;
            }
        }
    }

    if(*lastChar) while(*lastChar++);
    return lastChar - msdata + 1;
}

quint16 QMetaHost::computeMetaDataSize(const QMetaObject *meta)
{
    const uint *mdata = meta->d.data;

    uint revision = mdata[0];
    if(revision != 6)
        return 0;

    int metaSize = 15; //meta info + eod
    metaSize += mdata[2] * 2; //class info
    metaSize += mdata[4] * 5; //class methods
    metaSize += mdata[6] * 3; //class properties
    metaSize += mdata[8] * 4; //class enums
    uint eshift = mdata[9];
    for(uint i = 0; i < mdata[8]; ++i) {
        metaSize += mdata[eshift + i * 4 + 2] * 2; //class enum flags
    }
    metaSize += mdata[10] * 5; //class constructors

    return metaSize;
}

QObject *QMetaHost::findObjectByName(const QString& name)
{
    foreach(QObject *object, _remoteObjects.keys())
    {
        if(object && object->objectName() == name)
            return object;
    }

    return nullptr;
}

QObject *QMetaHost::getObject(const QString& name, QObject *client)
{
    QObject *object = findObjectByName(name);
    if(object)
    {
        auto om = _remoteObjects.find(object);
        ObjectMeta meta = (*om);
        if(meta.qualified && meta.client == client)
            return object;
        else
            return nullptr;
    }

    makeQuery(client, name, QueryObjectInfo, SIGNAL(gotObjectInfo()));

    object = findObjectByName(name);
    if(!object)
        return nullptr;

    auto om = _remoteObjects.find(object);
    ObjectMeta meta = (*om);
    if(meta.qualified && meta.client == client)
        return object;

	foreach(const QString& clsName, meta.classes)
	{
		auto c = _classes.find(clsName);
		if(c == _classes.end())
		{
            makeQuery(client, clsName, QueryClassInfo, SIGNAL(gotClassInfo()));

			auto c = _classes.find(clsName);
			if(c == _classes.end())
            {
                object->deleteLater();
                _remoteObjects.erase(om);
				return nullptr;
            }
		}
	}

	constructObject(object, meta.classes);
    return object;
}

void QMetaHost::prepareQuery(char *data, char **answer, quint8 type)
{
    //Send query object message
    int szlen = sizeof(quint16); //size of packet
    int tplen = sizeof(quint8); //size of type in header
    char *name = data;
    int nlen = strlen(name) + 1;
    quint64 msglen = szlen + tplen + nlen;
    quint16 packlen = tplen + nlen;

    char *msg = new char[msglen];
    char *ptr = msg;

    //Copy data to msg
    *((quint16 *)ptr) = packlen;
    ptr += szlen;
    *((quint8 *)ptr) = type;
    ptr += tplen;
    memcpy(ptr, name, nlen);

    *answer = msg;
}

void QMetaHost::makeQuery(QObject *client, const QString& name, 
    quint8 ctype, const char *signal)
{
    QByteArray nameBytes = name.toLocal8Bit();
    char *rawName = nameBytes.data();
    char *query = nullptr;
    prepareQuery(rawName, &query, ctype);
    QMetaEvent *me = new QMetaEvent(client, query);
    QCoreApplication::postEvent(client, me, Qt::HighEventPriority);

    QEventLoop loop;
    QTimer timer;
    connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
    connect(this, signal, &loop, SLOT(quit()));
    timer.start(3000);
    loop.exec();
}

bool QMetaHost::constructObject(QObject *object, const QStringList& classes)
{
    if(!classes.size())
        return false;

    auto latestClass = _classes.find(classes[0]);
    if(latestClass == _classes.end())
        return false;

    QMetaObject *metaClass = (*latestClass).metaObject;
    QProxyObject *proxy = static_cast<QProxyObject *>(object);
    proxy->setMetaObject(metaClass);
    

    QMetaObject *last = nullptr;
    foreach(const QString& clname, classes)
    {
        auto cl = _classes.find(clname);
        if(cl == _classes.end())
            return false;

        if(last)
        {
            QMetaObject *superClass = 
                const_cast<QMetaObject *>(last->d.superdata);

            if(!superClass)
                last->d.superdata = (*cl).metaObject;
        }

        last = (*cl).metaObject;
    }

    return true;
}

void QMetaHost::onDestroyed(QObject *client)
{
    int clientId = _clients.indexOf(client);
    _clients.removeAt(clientId);

    foreach(QObjectList *clients, _localObjects)
    {
        int idx = clients->indexOf(client);
        clients->removeAt(idx);
    }
}

bool QMetaHost::registerMetaClass(const QMetaObject *meta)
{
    bool result = true;

    while(meta)
    {
        QString className = metaName(meta);
        if(_classes.find(className) != _classes.end())
        {
            meta = meta->d.superdata;
            continue;
        }

        result &= checkRevision(meta);

        quint32 metaStringSize;
        result &= ((metaStringSize = computeMetaStringSize(meta)) >= 0);

        quint32 metaDataSize;
        result &= ((metaDataSize = computeMetaDataSize(meta)) >= 0);

        if(!result)
            return result;

        ClassMeta classMeta = { 
            const_cast<QMetaObject *>(meta),
            metaDataSize,
            metaStringSize
        };
        _classes.insert(className, classMeta);

        meta = meta->d.superdata;
    }

    return result;
}
