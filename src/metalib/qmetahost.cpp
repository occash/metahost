#include "qmetahost.h"
#include "qtcptransport.h"

#include <QMetaObject>
#include <QDebug>
#include <QDataStream>
#include <QIODevice>
#include <QMetaMethod>

#define SERVER_OK 1 
#define SERVER_BAD 0


//*******************Streams*******************************************
//Move to proto!!!
QDataStream &operator<<(QDataStream &out, const ClassMeta &classMeta)
{
    out << classMeta.dataSize;
    const uint *data = classMeta.metaObject->d.data;
    out.writeRawData((const char *)data, classMeta.dataSize * sizeof(uint));
    //Check the actual size of written bytes!!

    out << classMeta.stringSize;
    const char *string = classMeta.metaObject->d.stringdata;
    out.writeRawData(string, classMeta.stringSize);

    return out;
}

QDataStream &operator>>(QDataStream &in, ClassMeta &classMeta)
{
    if(!classMeta.metaObject)
        return in;

    quint32 dataSize;
    in >> dataSize;
    classMeta.dataSize = dataSize;
    classMeta.metaObject->d.data = new uint[dataSize];
    in.readRawData((char *)classMeta.metaObject->d.data, dataSize);

    quint32 stringSize;
    in >> stringSize;
    classMeta.stringSize = stringSize;
    classMeta.metaObject->d.stringdata = new char[stringSize];
    in.readRawData((char *)classMeta.metaObject->d.stringdata, stringSize);

    return in;
}

QDataStream &operator<<(QDataStream &out, const ObjectMeta &objMeta)
{
    out << objMeta.classInfo.size();
    for (int i = 0; i < objMeta.classInfo.size(); ++i)
    {
        out << objMeta.classInfo.at(i);
    }
    
    return out;
}

QDataStream &operator>>(QDataStream &in, ObjectMeta &objMeta)
{
    int size = 0;
    in >> size;

    QStringList classNames;

    for(int i = 0; i < size; ++i)
    {
        QString cname;
        in >> cname;
        objMeta.classInfo << cname;
    }

    return in;
}

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

	std::cout << "Exploited by occash!" << std::endl;

	/*for(int i = 0; i < _hosts.size(); ++i)
	{
		QMetaHost *host = _hosts.at(i);*/
	QMetaHost *host = _hosts;

		foreach(QString e, host->_objects.keys())
		{
			if(host->_objects.value(e).object == caller)
			{
				std::cout << "Object found" << std::endl;
				QByteArray answer;
				QDataStream out(&answer, QIODevice::ReadWrite);
				out << Command::EmitSignal;
				out << e;
				out << method_index;

				const QMetaObject *mo = caller->metaObject();
				QMetaMethod member = mo->method(method_index);
				QList<QByteArray> args = member.parameterTypes();
				out << args.size();
				
				for (int i = 0; i < args.count(); ++i) {
					const QByteArray &arg = args.at(i);
					int typeId = QMetaType::type(args.at(i).constData());
					QVariant argvar(typeId, argv[i + 1]);
					out << argvar;
					/*if (arg.endsWith('*') || arg.endsWith('&')) 
					{

					} 
					else if (typeId != QMetaType::Void) 
					{

					}*/
				}

				_hosts->_transport->broadcast(answer);
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
	qt_register_signal_spy_callbacks(callback);
	return true;
}

//QList<QMetaHost *> QMetaHost::_hosts = NULL;
QMetaHost * QMetaHost::_hosts = NULL;
bool QMetaHost::_initSpy = QMetaHost::initSignalSpy();

//**************************************Meta host*********************************

QMetaHost::QMetaHost(QTcpTransport *transport, QObject *parent)
    : QObject(parent),
    _transport(transport)
{
	//_hosts.append(this);
	_hosts = this;
    transport->setParent(this);
}

QMetaHost::~QMetaHost()
{
	//_hosts.takeAt(_hosts.indexOf(this));
}

bool QMetaHost::registerObject(const QString& name, QObject *object)
{
    bool result = true;
    const QMetaObject *meta = object->metaObject();
    if(!meta)
        return false;

    ObjectMeta objectMeta;
    objectMeta.object = object;

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
        objectMeta.classInfo.append(className);

        meta = meta->d.superdata;
    }

    _objects.insert(name, objectMeta);

    return result;
}

void QMetaHost::processCommand(QIODevice *client, QByteArray *data)
{
    QDataStream in(data, QIODevice::ReadOnly);
    in.setVersion(QDataStream::Qt_4_8);
    quint32 rawCommand = 0x00;
    in >> rawCommand;
    Command command = static_cast<Command>(rawCommand);

    QByteArray answer;
    QDataStream out(&answer, QIODevice::ReadWrite);

    switch(command)
    {
    case QueryObjectInfo:
    {
        QString objName;
        in >> objName;
        qDebug() << "Query object info:" << objName;

		out << ReturnObjectInfo;
        ObjectMap::iterator i = _objects.find(objName);
        if(i != _objects.end())
        {
            out << quint8(SERVER_OK);
            out << *i;
        }
        else
        {
            out << quint8(SERVER_BAD);
        }

        //Debug output
        qDebug() << "Answer to client:";
        for (int i = 0; i < answer.size(); ++i)
        {
            qDebug() << answer.at(i);
        }
    }
        break;
    case QueryClassInfo:
    {
        QString clsName;
        in >> clsName;
        qDebug() << "Query class info:" << clsName;

        ClassMap::iterator i = _classes.find(clsName);
        if(i != _classes.end())
        {
            out << quint8(SERVER_OK);
            out << *i;
        }
        else
        {
            out << quint8(SERVER_BAD);
        }

        //Debug output
        qDebug() << "Answer to client:";
        for (int i = 0; i < answer.size(); ++i)
        {
            qDebug() << answer.at(i);
        }
    }
        break;
    case CallMetaMethod:
        break;
    default:
        break;
    }

    _transport->write(client, answer);
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

	return nullptr;
}
