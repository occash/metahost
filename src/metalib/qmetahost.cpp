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
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
#include "qobject_p.h"
#endif

#include <QtGlobal>
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

#include <iostream>
#include <assert.h>

//Define version specific macros
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
#define QT_MALLOC(x) qMalloc(x)
#define QT_FREE(x) qFree(x)
#define QT_META_CONSTRUCT(id) QMetaType::construct(id)
#define QT_META_STRINGDATA(meta) meta->d.stringdata
#else
#define QT_MALLOC(x) qMallocAligned(x, 0)
#define QT_FREE(x) qFreeAligned(x)
#define QT_META_CONSTRUCT(id) QMetaType::create(id)
#define QT_META_STRINGDATA(meta) (const char *)((meta)->d.stringdata->data())
#endif

template<typename Tag, typename Tag::type M>
struct Rob {
  friend typename Tag::type get(Tag) {
    return M;
  }
};

struct QObject_p {
  typedef QScopedPointer<QObjectData> QObject::*type;
  friend type get(QObject_p);
};

template struct Rob<QObject_p, &QObject::d_ptr>;

#define HEADER_SIZE sizeof(quint16)

#define MOVE(ptr, size) ptr += size

#define SET(ptr, type, data) \
    *((type *)ptr) = data

#define GET(ptr, type, data) \
    data = *((type *)ptr)

#define WRITE(ptr, type, data) \
    *((type *)ptr) = data; \
    ptr += sizeof(type)

#define WRITECHUNK(ptr, data, size) \
    memcpy(ptr, data, size); \
    ptr += size

#define COPY(ptr, data, size) \
    memcpy(ptr, data, size)

#define READ(ptr, type) \
    *((type *)ptr); \
    ptr += sizeof(type)

#if Q_MOC_OUTPUT_REVISION == 67
struct MetaHeader
{
    uint revision;
    uint className;
    uint classInfoCount;
    uint classInfoData;
    uint methodCount;
    uint methodData;
    uint propertyCount;
    uint propertyData;
    uint enumeratorCount;
    uint enumeratorData;
    uint constructorCount;
    uint constructorData;
    uint flags;
    uint signalCount;
};

struct MetaMethod
{
    uint name;
    uint argc;
    uint parameters;
    uint tag;
    uint flags;
};

struct MetaProperty
{
    uint name;
    uint type;
    uint flags;
};

enum MetaDataFlags {
    IsUnresolvedType = 0x80000000,
    TypeNameIndexMask = 0x7FFFFFFF
};

inline const MetaHeader *header(const uint *data)
{
    return reinterpret_cast<const MetaHeader *>(data);
}

inline uint value(const uint *data, int index)
{
    return data[index];
}

inline uint value(const MetaHeader *head, int index)
{
    return (reinterpret_cast<const uint *>(head))[index];
}

inline const MetaMethod *method(const uint *data, int handle)
{
    return reinterpret_cast<const MetaMethod *>(data + handle);
}

inline const MetaProperty *metaprop(const uint *data, int handle)
{
    return reinterpret_cast<const MetaProperty *>(data + handle);
}
#endif

uint signalOffset(const QMetaObject *meta)
{
    const MetaHeader *h = header(meta->d.data);
    return h->signalCount;
}

uint methodOffset(const QMetaObject *meta)
{
    const MetaHeader *h = header(meta->d.data);
    return h->methodCount;
}

uint propertyOffset(const QMetaObject *meta)
{
    const MetaHeader *h = header(meta->d.data);
    return h->propertyCount;
}

int local(const QMetaObject **meta, int index, uint (* func)(const QMetaObject *meta))
{
    //Find methods offset
    int offset = 0;
    const QMetaObject *m = (*meta)->d.superdata;
    while (m) {
        offset += func(m);
        m = m->d.superdata;
    }

    //Compute local index
    while(*meta) {
        if(index >= offset)
            break;

        *meta = (*meta)->d.superdata;
        offset -= func(*meta);
    }

    return index - offset;
}

inline uint methodHandle(const QMetaObject *meta, int local_index, bool prop = false)
{
    return meta->d.data[prop ? 7 : 5] + ((prop ? 3 : 5) * local_index);
}

QList<QByteArray> methodArguments(const QMetaObject *meta, uint handle)
{
    QList<QByteArray> list;

    //Find method signature
    const char *signature = QT_META_STRINGDATA(meta) + meta->d.data[handle];

    //Parse params
    while (*signature && *signature != '(')
        ++signature;

    while (*signature && *signature != ')' && *++signature != ')') {
        const char *begin = signature;
        int level = 0;

        while (*signature && (level > 0 || *signature != ',') && *signature != ')') {
            if (*signature == '<')
                ++level;
            else if (*signature == '>')
                --level;
            ++signature;
        }

        list << QByteArray(begin, signature - begin);
    }

    return list;
}

bool writeRaw(QDataStream& out, int typeId, void *data)
{
    bool saved = QMetaType::save(out, typeId, data);
    if(!saved)
        qWarning() << "Cannot write method meta type" << QMetaType::typeName(typeId);

    return saved;
}

bool writeRawParams(QByteArray *ret, const QMetaObject *meta, int handle, void **argv)
{
    QDataStream argStream(ret, QIODevice::WriteOnly);
    bool result = true;

#if Q_MOC_OUTPUT_REVISION == 67
    const MetaMethod *metaMethod = method(meta->d.data, handle);
    for(uint i = 0; i < metaMethod->argc; ++i)
    {
        int typeId = value(meta->d.data, metaMethod->parameters + i + 1);
        if(typeId & IsUnresolvedType) {
            const char *typeName = (const char *)meta->d.stringdata[typeId & TypeNameIndexMask].data();
            typeId = QMetaType::type(typeName);
        }
        result &= writeRaw(argStream, typeId, argv[i + 1]);
    }
#else
    QList<QByteArray> args = methodArguments(meta, handle);
    for(int i = 0; i < args.size(); ++i)
    {
        int typeId = QMetaType::type(args.at(i).constData());
        result &= writeRaw(argStream, typeId, argv[i + 1]);
    }
#endif

    return result;
}

bool writeRawProp(QByteArray *ret, const QMetaObject *meta, int handle, void **argv)
{
    QDataStream argStream(ret, QIODevice::WriteOnly);
    int typeId;

#if Q_MOC_OUTPUT_REVISION == 67
    const MetaProperty *prop = metaprop(meta->d.data, handle);
    typeId = prop->type;
    if(typeId & IsUnresolvedType) {
        const char *typeName = (const char *)meta->d.stringdata[typeId & TypeNameIndexMask].data();
        typeId = QMetaType::type(typeName);
    }
#else
    const char *typeName = QT_META_STRINGDATA(meta) + meta->d.data[handle + 1];
    if (!typeName)
        return false;

    typeId = QMetaType::type(typeName);
#endif

    return writeRaw(argStream, typeId, argv[0]);
}

bool readRawParams(QByteArray *ret, const QMetaObject *meta, uint handle, void **argv)
{
    QDataStream argStream(ret, QIODevice::ReadOnly);

#if Q_MOC_OUTPUT_REVISION == 67
    const MetaMethod *metaMethod = method(meta->d.data, handle);
    for(uint i = 0; i < metaMethod->argc; ++i)
    {
        int typeId = value(meta->d.data, metaMethod->parameters + i + 1);
        if(typeId & IsUnresolvedType) {
            const char *typeName = (const char *)meta->d.stringdata[typeId & TypeNameIndexMask].data();
            typeId = QMetaType::type(typeName);
        }
        argv[i + 1] = QT_META_CONSTRUCT(typeId);
        bool loaded = QMetaType::load(argStream, typeId, argv[i + 1]);

        if(!loaded)
        {
            qWarning() << "Cannot read method meta type" << QMetaType::typeName(typeId);
            return false;
        }
    }
#else
    int paramNumber = 0;

    foreach(const QByteArray& typeName, methodArguments(meta, handle))
    {
        int typeId = QMetaType::type(typeName.constData());
        argv[paramNumber + 1] = QT_META_CONSTRUCT(typeId);
        bool loaded = QMetaType::load(argStream, typeId, argv[paramNumber + 1]);
        if(!loaded)
        {
            qWarning() << "Cannot read method meta type" << typeName;
            return false;
        }

        paramNumber++;
    }
#endif

    return true;
}

bool readRawProp(QByteArray *ret, const QMetaObject *meta, uint handle, void **argv)
{
    QDataStream argStream(ret, QIODevice::ReadOnly);
    int typeId;

    const MetaProperty *prop = metaprop(meta->d.data, handle);
    typeId = prop->type;

    if(typeId & IsUnresolvedType) {
        const char *typeName = (const char *)meta->d.stringdata[typeId & TypeNameIndexMask].data();
        typeId = QMetaType::type(typeName);
    }

    bool loaded = QMetaType::load(argStream, typeId, argv[0]);
    if (!loaded)
    {
        qWarning() << "Cannot read property meta type" << QMetaType::typeName(typeId);
        return false;
    }

    return true;
}

bool readRawReturn(QByteArray *ret, const QMetaObject *meta, int method_index, void *argv, bool prop = false)
{
    QDataStream argStream(ret, QIODevice::ReadOnly);

    int local_index = local(&meta, method_index, prop ? propertyOffset : methodOffset);
    uint handle = methodHandle(meta, local_index, prop);

#if Q_MOC_OUTPUT_REVISION == 67
    int metaType = QMetaType::Void;
    if(prop)
    {
        const MetaProperty *metaProperty = metaprop(meta->d.data, handle);
        metaType = metaProperty->type;
    }
    else
    {
        const MetaMethod *metaMethod = method(meta->d.data, handle);
        metaType = value(meta->d.data, metaMethod->parameters);
    }
    if(metaType != QMetaType::Void)
    {
        bool loaded = QMetaType::load(argStream, metaType, argv);
        if(!loaded)
        {
            qWarning() << "Cannot read returned meta type" << QMetaType::typeName(metaType);
            return false;
        }
    }
#else
    const char *retName = QT_META_STRINGDATA(meta) + meta->d.data[handle + (prop ? 1 : 2)];

    if(retName)
    {
        int typeId = QMetaType::type(retName);
        bool loaded = QMetaType::load(argStream, typeId, argv);
        if(!loaded)
        {
            qWarning() << "Cannot load meta type" << retName;
            return false;
        }
    }
#endif

    return true;
}

void freeParams(const QMetaObject *meta, int method_index, void **argv)
{
    int local_index = local(&meta, method_index, methodOffset);
    uint handle = methodHandle(meta, local_index);

#if Q_MOC_OUTPUT_REVISION == 67
    const MetaMethod *metaMethod = method(meta->d.data, handle);
    for(uint i = 0; i < metaMethod->argc; ++i)
    {
        int metaType = value(meta->d.data, metaMethod->parameters + i + 1);
        QMetaType::destroy(metaType, argv[i + 1]);
    }
#else
    int paramNumber = 0;
    foreach(const QByteArray& typeName, methodArguments(meta, handle))
    {
        int typeId = QMetaType::type(typeName.constData());
        QMetaType::destroy(typeId, argv[paramNumber + 1]);
        paramNumber++;
    }
#endif
}

void freeProp(const QMetaObject *meta, int method_index, void **argv)
{
    int local_index = local(&meta, method_index, methodOffset);
    uint handle = methodHandle(meta, local_index);
    const char *typeName = QT_META_STRINGDATA(meta) + meta->d.data[handle + 1];
    if (!typeName)
        return;

    int typeId = QMetaType::type(typeName);
    QMetaType::destroy(typeId, argv[1]);
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

void QMetaHost::hostCallback(QObject *caller, int method_index, void **argv)
{
    QMetaHost *host = nullptr;
	if(!(host = _host))
		return;

    auto i = host->_localObjects.find(caller);
    if(i == host->_localObjects.end())
        return;

    //Minimal answer size
    quint16 answerSize = sizeof(quint8) + sizeof(quint32) + 
        sizeof(int) + sizeof(quint16);

    //Write arguments using Qt meta system
    QByteArray argData;
    const QMetaObject *metaObject = caller->metaObject();
    int local_index = local(&metaObject, method_index, signalOffset);
    uint handle = methodHandle(metaObject, local_index);

    if(!writeRawParams(&argData, metaObject, handle, argv))
        return;

    //Adjust message size to store arguments
    answerSize += argData.size();

    //Allocate memory for answer
    char *msg = new char[HEADER_SIZE + answerSize];
    char *ptr = msg;

    //Set header size and type
    WRITE(ptr, quint16, answerSize);
    WRITE(ptr, quint8, EmitSignal);

    //Set generated object id
    WRITE(ptr, quint32, reinterpret_cast<quint32>(caller));
	
    //Set method index
    WRITE(ptr, int, method_index);

    //Check arguments size on remote side!
    SET(ptr, quint16, 0);

    if(argData.size() > 0)
    {
        //Set raw data size
        WRITE(ptr, quint16, argData.size());

        //Copy raw parameters data
        COPY(ptr, argData.data(), argData.size());
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
    if(_c == QMetaObject::RegisterMethodArgumentMetaType ||
        _c == QMetaObject::RegisterPropertyMetaType)
    {
        qDebug() << "Register types before calling to qt_metacall";
        return 1;
    }

    auto o = _remoteObjects.find(_o);
    if(o == _remoteObjects.end())
        return 1;

    int answerSize = sizeof(quint8) + sizeof(quint32) + 
        sizeof(QMetaObject::Call) + sizeof(int) + sizeof(quint16);

    QByteArray argData;
    const QMetaObject *meta = _o->metaObject();

    //decide whether it is property or method
    bool prop = (_c > QMetaObject::InvokeMetaMethod &&
        _c < QMetaObject::CreateInstance);
    int local_index = local(&meta, _id, prop ? propertyOffset : methodOffset);
    uint handle = methodHandle(meta, local_index, prop);

    if (_c == QMetaObject::InvokeMetaMethod)
    {
        if (!writeRawParams(&argData, meta, handle, _a))
            return 1;
    }
    else if (_c == QMetaObject::WriteProperty)
    {
        if (!writeRawProp(&argData, meta, handle, _a))
            return 1;
    }

    //Else no params
    answerSize += argData.size();

    //Allocate memory for answer
    char *msg = new char[HEADER_SIZE + answerSize];
    char *ptr = msg;

    //Set header size and type
    WRITE(ptr, quint16, answerSize);
    WRITE(ptr, quint8, CallMetaMethod);

    //Set qt_meta_call raw params
    WRITE(ptr, quint32, (*o).id);
    WRITE(ptr, QMetaObject::Call, _c);
    WRITE(ptr, int, _id);

    //Check size on remote side!!
    SET(ptr, quint16, quint16(0));

    if(argData.size() > 0)
    {
        //Set raw data size
        WRITE(ptr, quint16, argData.size());

        //Copy raw parameters data
        COPY(ptr, argData.data(), argData.size());
    }

    QMetaEvent *me = new QMetaEvent((*o).client, msg);
    QCoreApplication::postEvent((*o).client, me, Qt::HighEventPriority);

    ParamHolder holder;
    QEventLoop loop;
    QTimer timer;
    connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
    connect(&timer, SIGNAL(timeout()), &holder, SLOT(setTimeout()));

    connect(this, SIGNAL(gotMethodReturn(ReturnParam)),
        &holder, SLOT(setParam(ReturnParam)));
    connect(this, SIGNAL(gotMethodReturn(ReturnParam)), &loop, SLOT(quit()));

    timer.start(3000);
    //Wait until return to our function
    do 
    {
        loop.exec();
    } while ((holder.param.callType != _c ||
        holder.param.methodIndex != _id) &&
        !holder.timeout);
    
    if(holder.param.callType != _c ||
        holder.param.methodIndex != _id)
        return 1;

    bool isProp = (_c == QMetaObject::ReadProperty) ||
        ((_c >= QMetaObject::ResetProperty) && 
        (_c <= QMetaObject::QueryPropertyUser));

    if (!readRawReturn(&holder.param.returnArg, meta, _id, _a[0], isProp))
        return 1;

    return holder.param.returnId;
}

//**************************************Meta host*********************************

QMetaHost::QMetaHost(QObject *parent)
    : QObject(parent)
{
	_host = this;
}

QMetaHost::~QMetaHost()
{
    _host = nullptr;
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
    {
        _localObjects.insert(object, new QObjectList());
        object->installEventFilter(this);
    }

    return result;
}

//Processing stuff

void QMetaHost::processQueryObjectInfo(QObject *client, char *data, char **answer)
{
    QString objectName(data);

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

            connect(client, SIGNAL(destroyed(QObject *)),
                this, SLOT(onDestroyed(QObject *)));

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
            answerSize += strlen(QT_META_STRINGDATA(meta)) + 1;
            meta = const_cast<QMetaObject *>(meta->d.superdata);
        }
    }

    //Allocate memory for answer
    int headerSize = sizeof(quint16);
    *answer = new char[headerSize + answerSize];
    char *ptr = *answer;

    //Set header size
    WRITE(ptr, quint16, answerSize);
    //Set header type
    quint8 answerType = ReturnObjectInfo;
    WRITE(ptr, quint8, answerType);
    //Set name of object
    WRITECHUNK(ptr, data, objectName.size() + 1);
    //Set generated object id
    WRITE(ptr, quint32, id);

    //Set class names related to object
    if(object)
    {
        QMetaObject *meta = const_cast<QMetaObject *>(
            object->metaObject());
        while(meta)
        {
            const char *className = QT_META_STRINGDATA(meta);
            int nameLength = strlen(className) + 1;
            WRITECHUNK(ptr, className, nameLength);
            meta = const_cast<QMetaObject *>(meta->d.superdata);
        }
    }

    //Add terminating zero
    SET(ptr, char, '\0');
}

void QMetaHost::processReturnObjectInfo(QObject *client, char *data, char **answer)
{
    QString objectName(data);
    char *ptr = data;
    *answer = 0;

    MOVE(ptr, objectName.size() + 1);
    quint32 id;
    GET(ptr, quint32, id);
    //Remote side didn't found object
    if(!id)
        return;

    //Construct empty ObjectMeta
    ObjectMeta objectMeta;
    objectMeta.id = id;
    objectMeta.qualified = true;
    objectMeta.client = client;
    MOVE(ptr, sizeof(quint32));

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
    char *newData = new char[stringSize];
#if Q_MOC_OUTPUT_REVISION == 67
    classInfo->d.stringdata = (QByteArrayData *)newData;
#else
    classInfo->d.stringdata = newData;
#endif
    memcpy(newData, data, stringSize);
    data += stringSize;

    classMeta.metaObject = classInfo;
    _classes.insert(className, classMeta);

    emit gotClassInfo();
}

void QMetaHost::processEmitSignal(QObject *client, char *data, char **answer)
{
    Q_UNUSED(client);
    Q_UNUSED(answer);

    //Read object Id
    quint32 id = READ(data, quint32);
    auto o = _remoteIds.find(id);
    if(o == _remoteIds.end())
        return;

    //Read raw method index
    int method_index = READ(data, int);
    const QMetaObject *meta = (*o)->metaObject();
    const QMetaObject *m = meta;
    int local_index = local(&m, method_index, signalOffset);
    uint handle = methodHandle(m, local_index);

    //Allocate memory for raw params (max 10 arguments for call)
    void **argv = (void **)QT_MALLOC(10 * sizeof(void *));

    //Signal never returns, so left field blank
    memset(argv, 0, 10 * sizeof(void *));

    //Read raw argument data and fill argv
    quint16 dataSize = *((quint16 *)data);
    data += sizeof(quint16);

    QByteArray argData = QByteArray::fromRawData(data, dataSize);
    if(!readRawParams(&argData, m, handle, argv))
    {
        freeParams(meta, method_index, argv);
        QT_FREE(argv);
        return;
    }
    
    //Activate signal
    QMetaObject::activate((*o), m, local_index, argv);

    //deallocate params
    freeParams(meta, method_index, argv);
    QT_FREE(argv);
}

void QMetaHost::processCallMetaMethod(QObject *client, char *data, char **answer)
{
    Q_UNUSED(client);
    Q_UNUSED(answer);

    //First check if object is registered
    //_o, _c, _id, _a
    QObject *object = READ(data, QObject *);
    QMetaObject::Call c = READ(data, QMetaObject::Call);
    int id = READ(data, int);

    if(c == QMetaObject::RegisterMethodArgumentMetaType ||
        c == QMetaObject::RegisterPropertyMetaType)
    {
        prepareReturnMetaMethod(reinterpret_cast<quint32>(object), c, id,
            nullptr, QMetaType::Void, 1, answer);
        return; //Send no such method
    }

    auto o = _localObjects.find(object);
    if(o == _localObjects.end()) 
    {
        prepareReturnMetaMethod(reinterpret_cast<quint32>(object), c, id,
            nullptr, QMetaType::Void, 1, answer);
        return; //Send no such method
    }

    const QMetaObject *meta = object->metaObject();
    const QMetaObject *m = meta;

    //decide whether it is property or method
    bool prop = (c > QMetaObject::InvokeMetaMethod &&
        c < QMetaObject::CreateInstance);
    bool writeProp = (c == QMetaObject::WriteProperty);

    //Allocate memory for raw params
    int local_index = local(&m, id, prop ? propertyOffset : methodOffset);
    int handle = methodHandle(m, local_index, prop);

#if Q_MOC_OUTPUT_REVISION == 67
    int arrayLen = 1;
    int typeId = QMetaType::Void;
    const MetaMethod *metaMethod = nullptr;
    const MetaProperty *metaProperty = nullptr;

    if(prop)
    {
        metaProperty = metaprop(meta->d.data, handle);
        typeId = metaProperty->type;
    }
    else
    {
        metaMethod = method(meta->d.data, handle);
        arrayLen += metaMethod->argc + 1;
        typeId = value(meta->d.data, metaMethod->parameters);
    }

    void **argv = (void **)QT_MALLOC(arrayLen * sizeof(void *));
    memset(argv, 0, arrayLen * sizeof(void *));
        
    if(typeId != QMetaType::Void)
        argv[0] = QT_META_CONSTRUCT(typeId);
#else
    QList<QByteArray> paramTypes;
    if(!prop)
        paramTypes = methodArguments(meta, handle);
    int arrayLen = paramTypes.size() + 1;
    void **argv = (void **)QT_MALLOC(arrayLen * sizeof(void *));
    memset(argv, 0, arrayLen * sizeof(void *));
    
    const char *retName = QT_META_STRINGDATA(meta) + m->d.data[handle + (prop ? 1 : 2)];

    int typeId = QMetaType::Void;
    if(retName)
    {
        typeId = QMetaType::type(retName);
        argv[0] = QT_META_CONSTRUCT(typeId);
    }
#endif

    //Read raw parameters
    quint16 dataSize = READ(data, quint16);
    QByteArray argData = QByteArray::fromRawData(data, dataSize);
    if (!prop)
    {
        if (!readRawParams(&argData, meta, handle, argv))
        {
            prepareReturnMetaMethod(reinterpret_cast<quint32>(object), c, id,
                nullptr, QMetaType::Void, 1, answer);
            freeParams(meta, id, argv);
            QT_FREE(argv);
            return;
        }
    }
    if (writeProp)
    {
        if (!readRawProp(&argData, meta, handle, argv))
        {
            prepareReturnMetaMethod(reinterpret_cast<quint32>(object), c, id,
                nullptr, QMetaType::Void, 1, answer);
            freeProp(meta, id, argv);
            QT_FREE(argv);
            return;
        }
    }

    //Call method
    int returnId = QMetaObject::metacall(object, c, id, argv);

    //Prepare answer to caller side
    prepareReturnMetaMethod(reinterpret_cast<quint32>(object), c, id,
        argv, typeId, returnId, answer);
    if (writeProp)
        freeProp(meta, id, argv);
    else if (!prop)
        freeParams(meta, id, argv);

    if(typeId != QMetaType::Void)
        QMetaType::destroy(typeId, argv[0]);

    QT_FREE(argv);
}

void QMetaHost::prepareReturnMetaMethod(quint32 o, QMetaObject::Call c, 
    int id, void **argv, int typeId, int ret, char **answer)
{
    int answerSize = sizeof(quint8) + sizeof(quint32) + 
        sizeof(QMetaObject::Call) + sizeof(int) * 2 + sizeof(quint16);

    QByteArray argData;
    if(typeId != QMetaType::Void)
    {
        QDataStream argStream(&argData, QIODevice::WriteOnly);
        bool saved = QMetaType::save(argStream, typeId, argv[0]);
        if(!saved) 
            ret = 1; //Send no such method
    }

    answerSize += argData.size();
    //Allocate memory for answer
    *answer = new char[HEADER_SIZE + answerSize];
    char *ptr = *answer;

    //Set header size and type
    WRITE(ptr, quint16, answerSize);
    WRITE(ptr, quint8, ReturnMetaMethod);

    //Set qt_meta_call raw params
    WRITE(ptr, quint32, o);
    WRITE(ptr, QMetaObject::Call, c);
    WRITE(ptr, int, id);

    WRITE(ptr, int, ret);

    //Check size on remote side!!
    SET(ptr, quint16, 0);

    if(argData.size() > 0)
    {
        //Set raw data size
        WRITE(ptr, quint16, argData.size());

        //Copy raw parameters data
        COPY(ptr, argData.data(), argData.size());
    }
}

void QMetaHost::processReturnMetaMethod(QObject *client, char *data, char **answer)
{
    Q_UNUSED(client);
    Q_UNUSED(answer);

    ReturnParam param;
    param.returnArg = nullptr;
    param.returnId = 1;

    quint32 objectId = READ(data, quint32);

    auto o = _remoteIds.find(objectId);
    if(o == _remoteIds.end()) {
        emit gotMethodReturn(param);
        return;
    }

    QMetaObject::Call c = READ(data, QMetaObject::Call);
    int id = READ(data, int);
    int retId = READ(data, int);
    if(retId > 0) {
        emit gotMethodReturn(param);
        return; //Die fast
    }

    quint16 dataSize = READ(data, quint16);
    QByteArray argData(data, dataSize);

    param.callType = c;
    param.methodIndex = id;
    param.returnId = retId;
    param.returnArg = argData;

    emit gotMethodReturn(param);
}

void QMetaHost::processCommand(QObject *client, char *data)
{
    char *ptr = data;
    quint8 command = READ(data, quint8);
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

bool QMetaHost::eventFilter(QObject *o, QEvent *e)
{
    if(e->type() == QEvent::DynamicPropertyChange)
    {
        QDynamicPropertyChangeEvent *de =
                dynamic_cast<QDynamicPropertyChangeEvent *>(e);
        if(de)
        {
            /*QObjectData *ptr = ((*o).*get(QObject_p())).data(); // Doh!
            QObjectPrivate *ptr_p = dynamic_cast<QObjectPrivate *>(ptr);
            qDebug() << ptr_p->extraData->propertyNames.at(0);
            qDebug() << "Property" << de->propertyName() << "changed";*/
            //TODO: send message of property changing to client/server
        }
    }

    return false;
}

bool QMetaHost::checkRevision(const QMetaObject *meta)
{
    quint32 revision = header(meta->d.data)->revision;
    if(!(revision == 6 || revision == 7)) {
        qDebug() << "Error: QMetaHost only compatible with revision 6 or 7.";
        return false;
    }

    return true;
}

QString QMetaHost::metaName(const QMetaObject *meta)
{
#if Q_MOC_OUTPUT_REVISION == 67
    quint32 handle = header(meta->d.data)->className;
    return (const char *)meta->d.stringdata[handle].data();
#else
    const char *msdata = QT_META_STRINGDATA(meta);
    return msdata + header(meta->d.data)->className;
#endif
}

quint16 QMetaHost::computeMetaStringSize(const QMetaObject *meta)
{
    Q_ASSERT(meta->d.data != NULL);
    Q_ASSERT(meta->d.stringdata != NULL);

    const char *lastChar = NULL;

#if Q_MOC_OUTPUT_REVISION == 67
    int dataOffset = meta->d.stringdata[0].offset;
    int dataNumber = dataOffset / sizeof(QByteArrayData);

    lastChar = (char *)meta->d.stringdata + dataOffset;
    for(int i = 0; i < dataNumber; ++i)
    {
        if(meta->d.stringdata[i].data() > lastChar)
            lastChar = (char *)meta->d.stringdata[i].data();
    }
#else
    const uint *mdata = meta->d.data;
    const char *msdata = QT_META_STRINGDATA(meta);
    const MetaHeader *head = header(mdata);
    
    lastChar = msdata;
    
    {//Class name
        if(msdata + head->className > lastChar)
            lastChar = msdata + head->className;
    }

    {//Class info
        uint cinum = head->classInfoCount;
        uint cishift = head->classInfoData;
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
        uint cmnum = head->methodCount;
        uint cmshift = head->methodData;
        for(uint i = 0; i < cmnum; ++i) {
            for(uint j = 0; j < 4; ++j) {
                const char *cmsdata = msdata + mdata[cmshift + i * 5 + j];
                if(cmsdata > lastChar)
                    lastChar = cmsdata;
            }
        }
    }

    {//Class properties
        uint cpnum = head->propertyCount;
        uint cpshift = head->propertyData;
        for(uint i = 0; i < cpnum; ++i) {
            for(uint j = 0; j < 2; ++j) {
                const char *cpsdata = msdata + mdata[cpshift + i * 3 + j];
                if(cpsdata > lastChar)
                    lastChar = cpsdata;
            }
        }
    }

    {//Class enums/sets
        uint cenum = head->enumeratorCount;
        uint ceshift = head->enumeratorData;
        for(uint i = 0; i < cenum; ++i) {
            const char *cename = msdata + mdata[ceshift + i * 4];
            if(cename > lastChar)
                lastChar = cename;

            uint cecount = mdata[ceshift + i * 4 + 2];
            uint cedata = mdata[ceshift + i * 4 + 3];
            for(uint j = 0; j < cecount; ++j) {
                const char *ceval = msdata + mdata[cedata + j * 2];
                if(ceval > lastChar)
                    lastChar = ceval;
            }
        }
    }

    {//Class constructors
        uint ccnum = head->constructorCount;
        uint ccshift = head->constructorData;
        for(uint i = 0; i < ccnum; ++i) {
            for(uint j = 0; j < 4; ++j) {
                const char *ccsdata = msdata + mdata[ccshift + i * 5 + j];
                if(ccsdata > lastChar)
                    lastChar = ccsdata;
            }
        }
    }
#endif

    if(*lastChar) while(*lastChar++);

#if Q_MOC_OUTPUT_REVISION == 67
    return lastChar - (char *)meta->d.stringdata;
#else
    return lastChar - msdata + 1;
#endif
}

quint16 QMetaHost::computeMetaDataSize(const QMetaObject *meta)
{
    const MetaHeader *head = header(meta->d.data);

    quint16 metaSize = 15; //meta info + eod
    metaSize += head->classInfoCount * 2; //class info
    metaSize += head->methodCount * 5; //class methods
    metaSize += head->propertyCount * 3; //class properties
    metaSize += head->enumeratorCount * 4; //class enums
    uint eshift = head->enumeratorData;
    for(uint i = 0; i < head->enumeratorCount; ++i) {
        metaSize += value(head, eshift + i * 4 + 2) * 2; //class enum flags
    }
    metaSize += head->constructorCount * 5; //class constructors

#if Q_MOC_OUTPUT_REVISION == 67
    metaSize += head->methodCount; //Return type
    for(uint i = 0; i < head->methodCount; ++i)
    {
        metaSize += value(head, head->methodData + 5 * i + 1) * 2; //Param types + names
    }
    metaSize += head->propertyCount;
    metaSize += head->constructorCount;
    for (uint i = 0; i < head->constructorCount; ++i)
    {
        metaSize += value(head, head->constructorData + i * 5 + 1) * 2;
    }
#endif

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

    if(constructObject(object, meta.classes))
        object->installEventFilter(this);

    return object;
}

void QMetaHost::prepareQuery(char *data, char **answer, quint8 packetType)
{
    //Send query object message
    int dataLength = strlen(data) + 1;
    quint16 packetSize = sizeof(quint8) + dataLength;

    char *msg = new char[packetSize + sizeof(quint16)];
    char *ptr = msg;

    //Copy data to msg
    WRITE(ptr, quint16, packetSize);
    WRITE(ptr, quint8, packetType);
    COPY(ptr, data, dataLength);

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
