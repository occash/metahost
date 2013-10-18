#include "qmetahost.h"
#include "qtcptransport.h"
#include "qproxyobject.h"

#include <QMetaObject>
#include <QDebug>
#include <QDataStream>
#include <QIODevice>
#include <QMetaMethod>
#include <QEventLoop>
#include <QTimer>

#define SERVER_OK 1 
#define SERVER_BAD 0

//*********************************************************************
//**************Private hooks************************************
//********************************************************************


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

void QMetaHost::hostBeginCallback(QObject *caller, int method_index, void **argv)
{
	if(!_hosts)
		return;

	//std::cout << "Exploited by occash!" << std::endl;

	/*for(int i = 0; i < _hosts.size(); ++i)
	{
		QMetaHost *host = _hosts.at(i);*/
	QMetaHost *host = _hosts;

		foreach(QString e, host->_objects.keys())
		{
			if(host->_objects.value(e).object == caller)
			{
				if(host->_objects.value(e).remote)
					return;

				std::cout << "Object found" << std::endl;
				QByteArray answer;
				QDataStream out(&answer, QIODevice::ReadWrite);
				out << EmitSignal;
				out << e;

				const QMetaObject *mo = caller->metaObject();

				int parent = 0;
				const QMetaObject *m = mo;
				while (m) {
					if(method_index >= m->methodOffset())
						break;

					parent++;
					m = m->d.superdata;
				}

				int local_index = method_index - m->methodOffset();
				out <<parent;
				out << local_index;

				QMetaMethod member = mo->method(method_index);
				QList<QByteArray> args = member.parameterTypes();
				out << args.size();
				
				for (int i = 0; i < args.count(); ++i) {
					//Check if type is star type
					//Deep copy is needed
					const QByteArray &arg = args.at(i);
					int typeId = QMetaType::type(args.at(i).constData());
					QVariant argvar(typeId, argv[i + 1]);
					//out << argvar;
					/*if (arg.endsWith('*') || arg.endsWith('&')) 
					{

					} 
					else if (typeId != QMetaType::Void) 
					{

					}*/
				}

				//_hosts->_transport->broadcast(answer);
			}
		}
	//}
	
	/*QObjectMap::iterator _caller = host->_registered.begin();
	for(;_caller != host->_registered.end(); ++_caller) 
	{
		if(*_caller == caller)
		{

		}
	}*/
}

bool QMetaHost::initSignalSpy()
{
	QSignalSpyCallbackSet callback;
	callback.signal_begin_callback = QMetaHost::hostBeginCallback;
	callback.signal_end_callback = nullptr;
	callback.slot_begin_callback = nullptr;
	callback.slot_end_callback = nullptr;
	qt_register_signal_spy_callbacks(callback);
	return true;
}

//QList<QMetaHost *> QMetaHost::_hosts = NULL;
QMetaHost * QMetaHost::_hosts = NULL;
bool QMetaHost::_initSpy = QMetaHost::initSignalSpy();

//**************************************Meta host*********************************

QMetaHost::QMetaHost(QObject *parent)
    : QObject(parent)
{
	//_hosts.append(this);
	_hosts = this;
}

QMetaHost::~QMetaHost()
{
	//_hosts.takeAt(_hosts.indexOf(this));
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

    static quint32 _id = 1;
    _localObjects.insert(_id++, object);

    return result;
}

void QMetaHost::processQueryObjectInfo(char *data, char **answer, int *answerSize)
{
    QString objName = data;
    qDebug() << "Query object info:" << objName;

    //Locate the desired objects
    QObject *destObject = nullptr;
    quint32 destId = 0;
    auto i = _localObjects.begin();
    for(; i != _localObjects.end(); ++i)
    {
        if((*i)->objectName() == objName) {
            destObject = *i;
            break;
        }
    }

    //Compute size of packet required
    //minimal size of header
    int destSize = sizeof(quint8) + sizeof(quint32) + 1; 
    quint8 type = ReturnObjectInfo;
    if(destObject)
    {
        QMetaObject *meta = const_cast<QMetaObject *>(
            destObject->metaObject());
        while(meta)
        {
            destSize += strlen(meta->d.stringdata) + 1;
            meta = const_cast<QMetaObject *>(meta->d.superdata);
        }
    }

    //Allocate memory for answer
    *answer = new char[destSize];
    char *ptr = *answer;

    //Set header type
    *((quint8 *)ptr) = type;
    ptr += sizeof(quint8);
    //Set generated object id
    *((quint32 *)ptr) = destId;
    ptr += sizeof(quint32);
    //Set class names related to object
    if(destObject)
    {
        QMetaObject *meta = const_cast<QMetaObject *>(
            destObject->metaObject());
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

    *answerSize = destSize;
}

void QMetaHost::processReturnObjectInfo(char *data)
{
    ObjectMeta objectMeta;
    objectMeta.id = *((quint32 *)data);
    objectMeta.qualified = true;

    data += sizeof(quint8);
    if(objectMeta.id) 
    {
        QString className(data);
        auto i = _classes.find(className);
        if(i == _classes.end())
            objectMeta.qualified = false;

        objectMeta.classes << className;
        data += className.size() + 1;
    }

    if(objectMeta.qualified)
    {
        QList<ClassMeta> metas;
        foreach(const QString& clname, objectMeta.classes)
        {
            auto cl = _classes.find(clname);
            metas.append(*cl);
        }
        /*objMeta.object = new QProxyObject(metas);
        objMeta.remote = true;*/
    }

    emit gotObjectInfo();
}

void QMetaHost::processCommand(char *data, int size)
{
    quint8 command = *((quint8 *)data);
    data += sizeof(quint8);
    char *answer = nullptr;
    int answerSize = 0;

    switch(command)
    {
    case QueryObjectInfo:
		processQueryObjectInfo(data, &answer, &answerSize);
        break;
	case ReturnObjectInfo:
		{
			
		}
		return;
    case QueryClassInfo:
		{
			QString clsName;
			in >> clsName;
			qDebug() << "Query class info:" << clsName;

			out << ReturnClassInfo;
			ClassMap::iterator i = _classes.find(clsName);
			if(i != _classes.end())
			{
				out << quint8(SERVER_OK);
				out << clsName;
				out << *i;
			}
			else
			{
				out << quint8(SERVER_BAD);
			}
		}
        break;
	case ReturnClassInfo:
		{
			quint8 serverOk = 0;
			in >> serverOk;
			if(!serverOk)
				return;

			QString clsName;
			in >> clsName;
			ClassMeta clsMeta;
			clsMeta.metaObject = new QMetaObject();
			in >> clsMeta;

			_classes.insert(clsName, clsMeta);

			emit gotClassInfo();
		}
		return;
    case CallMetaMethod:
        return;
	case EmitSignal:
		{
			QString objName;
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

			QMetaObject::activate((*o).object, metaObject, method_index, 0);
		}
		return;
    default:
        return;
    }

//    _transport->write(client, answer);
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

int QMetaHost::computeMetaStringSize(const QMetaObject *meta)
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

int QMetaHost::computeMetaDataSize(const QMetaObject *meta)
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

QObject *QMetaHost::getObject(const QString& name)
{
	ObjectMap::iterator i = _objects.find(name);
	if(i != _objects.end())
		return (*i).object;

	QByteArray data;
	QDataStream outData(&data, QIODevice::WriteOnly);
	outData.setVersion(QDataStream::Qt_4_8);

	outData << QueryObjectInfo;
	outData << name;

//	_transport->broadcast(data);
	
	QEventLoop loop;
	QTimer timer;
	connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
	connect(this, SIGNAL(gotObjectInfo()), &loop, SLOT(quit()));
	timer.start(3000);
	loop.exec();

	auto o = _objects.find(name);
	if(o != _objects.end())
	{
		if((*o).fullQuilified)
			return (*o).object;
	} else
		return nullptr;

	ObjectMeta ometa = (*o);
	foreach(const QString& clsName, ometa.classInfo)
	{
		auto c = _classes.find(clsName);
		if(c == _classes.end())
		{
			QByteArray data;
			QDataStream outData(&data, QIODevice::WriteOnly);
			outData.setVersion(QDataStream::Qt_4_8);

			outData << QueryClassInfo;
			outData << clsName;

//			_transport->broadcast(data);

			QEventLoop loop;
			QTimer timer;
			connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
			connect(this, SIGNAL(gotClassInfo()), &loop, SLOT(quit()));
			timer.start(10000);
			loop.exec();

			auto c = _classes.find(clsName);
			if(c == _classes.end())
				return nullptr;
		}
	}

	QList<ClassMeta> metas;
	foreach(const QString& clname, (*o).classInfo)
	{
		auto cl = _classes.find(clname);
		metas.append(*cl);
	}
	(*o).object = new QProxyObject(metas);
	(*o).remote = true;
	(*o).fullQuilified = true;

	return (*o).object;
}
