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

    //Allocate memory for answer
    int headerSize = sizeof(quint16);
    quint16 answerSize = sizeof(quint8) + sizeof(quint32) * 2;

	const QMetaObject *metaObject = caller->metaObject();
	QMetaMethod metaMethod = metaObject->method(method_index);
	QList<QByteArray> paramTypes = metaMethod.parameterTypes();
    QByteArray argData;
    QDataStream argStream(&argData, QIODevice::WriteOnly);
				
	for(int i = 0; i < paramTypes.size(); ++i)
    {
        const QByteArray& typeName = paramTypes.at(i);
        int typeId = QMetaType::type(typeName.constData());
        bool saved = QMetaType::save(argStream, typeId, argv[i]);
        if(!saved)
        {
            qWarning() << "Cannot read meta type" << typeName;
            return;
        }
    }

    answerSize += argData.size();
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
    *((quint32 *)ptr) = method_index;
    ptr += sizeof(quint32);

    //Copy raw parameters data
    memcpy(ptr, argData.data(), argData.size());

    foreach(QObject *client, *(*i))
    {
        QMetaEvent *me = new QMetaEvent(client, msg);
        QCoreApplication::postEvent(client, me, Qt::HighEventPriority);
    }
}

void QMetaHost::signalCallback(QObject *caller, int method_index, void **argv)
{
    hostCallback(caller, method_index, argv);
}

void QMetaHost::slotCallback(QObject *caller, int method_index, void **argv)
{
    //hostCallback(caller, method_index, argv, CallMetaMethod);
}

bool QMetaHost::initSignalSpy()
{
	QSignalSpyCallbackSet callback;
	callback.signal_begin_callback = QMetaHost::signalCallback;
	callback.signal_end_callback = nullptr;
	callback.slot_begin_callback = QMetaHost::slotCallback;
	callback.slot_end_callback = nullptr;
	qt_register_signal_spy_callbacks(callback);
	return true;
}

QMetaHost * QMetaHost::_host = nullptr;
bool QMetaHost::_initSpy = QMetaHost::initSignalSpy();

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
    bool result = true;
    const QMetaObject *meta = object->metaObject();
    if(!meta)
        return false;

    while(meta)
    {
        QString className = metaName(meta);
        if(_classes.find(className) != _classes.end())
            continue;

        result &= checkRevision(meta);

        quint32 metaStringSize;
        result &= ((metaStringSize = computeMetaStringSize(meta)) >= 0);

        quint32 metaDataSize;
        result &= ((metaDataSize = computeMetaDataSize(meta)) >= 0);

        if(!result)
            return result;

        ClassMeta classMeta = { 
            const_cast<QMetaObject *>(meta), 
            metaStringSize, 
            metaDataSize 
        };
        _classes.insert(className, classMeta);

        meta = meta->d.superdata;
    }

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

    auto i = _classes.find(className);
    if(i != _classes.end())
    {
        answerSize += strlen((*i).metaObject->d.stringdata); //no +1 because it has added earlier
        answerSize += sizeof(quint32) * 2;
        answerSize += (*i).dataSize;
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

    *ptr = '\0'; //Found flag (false by default)

    //Copy meta information to message
    if(i != _classes.end())
    {
        memcpy(ptr, data, className.size() + 1);
        ptr += className.size() + 1;

        *((quint16 *)ptr) = (*i).dataSize;
        ptr += sizeof(quint16);

        memcpy(ptr, (*i).metaObject->d.data, (*i).dataSize);
        ptr += (*i).dataSize;

        *((quint16 *)ptr) = (*i).stringSize;
        ptr += sizeof(quint16);

        memcpy(ptr, (*i).metaObject->d.stringdata, (*i).stringSize);
        ptr += (*i).stringSize;
    }
}

void QMetaHost::processReturnClassInfo(QObject *client, char *data, char **answer)
{
    bool found = *((bool *)data);

    //Remote side didn't found class definition
    if(!found)
        return;

    QString className(data);
    data += className.size() + 1;

    ClassMeta classMeta;
    QMetaObject *classInfo = new QMetaObject();
    
    quint16 dataSize = *((quint16 *)data);
    classMeta.dataSize = dataSize;
    data += sizeof(quint16);
    classInfo->d.data = new uint[dataSize];
    memcpy((void *)classInfo->d.data, data, dataSize * sizeof(uint));
    data += dataSize;

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
                	//Move to signal handler
    /*quint8 parent = 0;
	const QMetaObject *m = mo;
	while (m) {
		if(method_index >= m->methodOffset())
			break;

		parent++;
		m = m->d.superdata;
	}

    quint32 local_index = method_index - m->methodOffset();*/
			/*QString objName;
			in >> objName;

			int parents;
			in >> parents;

			int method_index;
			in >> method_index;

			int numArgs;
			in >> numArgs;

			auto o = _objects.find(objName);
			if(o == _objects.end())
				return;

			ObjectMeta meta = (*o);
			const QMetaObject *metaObject = meta.object->metaObject();
			for(int i = 0; i < parents; ++i) {
				metaObject = metaObject->d.superdata;
			}

			QMetaObject::activate((*o).object, metaObject, method_index, 0);*/
}

void QMetaHost::processCommand(QObject *client, char *data)
{
    char *ptr = data;
    quint8 command = *((quint8 *)data);
    data += sizeof(quint8);
    char *answer = nullptr;
    int answerSize = 0;

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
    return lastChar - msdata;
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
    metaSize += mdata[6] * 4; //class properties
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

    makeQuery(client, name, QueryObjectInfo);

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
            makeQuery(client, clsName, QueryClassInfo);

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

void QMetaHost::makeQuery(QObject *client, const QString& name, quint8 ctype)
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
    connect(this, SIGNAL(gotObjectInfo()), &loop, SLOT(quit()));
    timer.start(3000);
    loop.exec();
}

void QMetaHost::constructObject(QObject *object, const QStringList& classes)
{
    QMetaObject *last = nullptr;
    QMetaObject *first = nullptr;
    QProxyObject *proxy = static_cast<QProxyObject *>(object);

    foreach(const QString& clname, classes)
    {
        auto cl = _classes.find(clname);

        if(last)
        {
            QMetaObject *superClass = 
                const_cast<QMetaObject *>(last->d.superdata);

            if(!superClass)
                superClass = (*cl).metaObject;
        }
        else
            first = (*cl).metaObject;

        last = (*cl).metaObject;
    }

    proxy->setMetaObject(first);
}
