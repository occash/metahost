#include <QtCore/QCoreApplication>
#include <QtCore/QMetaobject>
#include <QtCore/QDebug>

#include "moctest.h"

/*void remoteCall(QObject *_o, QMetaObject::Call _c, int _id, void **_arg);

struct RemoteExtraData : public QMetaObjectExtraData
{
	RemoteExtraData(QMetaObjectExtraData *extra, QMetaObject *host)
		: hostInfo(host),
		basicFunc(extra->static_metacall),
		oldExtra(extra)
	{
		objects = extra->objects;
		static_metacall = remoteCall;
	}

	QMetaObjectExtraData *oldExtra;
	QMetaObject *hostInfo;
	StaticMetacallFunction basicFunc;
};

void remoteCall(QObject *_o, QMetaObject::Call _c, int _id, void **_arg)
{
	QMetaObject *meta = const_cast<QMetaObject *>(_o->metaObject());
	RemoteExtraData *extra = 
		reinterpret_cast<RemoteExtraData *>(
		const_cast<void *>(meta->d.extradata));
	qDebug() << "Exploited by occash";
	if(extra) {
		extra->basicFunc(_o, _c, _id, _arg);
	}
}

bool prepareForRemote(QObject *obj)
{
	QMetaObject *meta = const_cast<QMetaObject *>(obj->metaObject());
	QMetaObjectExtraData *extra = 
		reinterpret_cast<QMetaObjectExtraData *>(
		const_cast<void *>(meta->d.extradata));
	//meta->d.extradata = new RemoteExtraData(extra, meta);

	meta->d.superdata = &QObject::staticMetaObject;
	meta->d.data = qt_meta_data;
	meta->d.stringdata = qt_meta_stringdata;
	QMetaObjectExtraData extraData;
	extraData.objects = 0;
	extraData.static_metacall = qt_static_metacall;
	meta->d.extradata = &extraData;

	return true;
}*/

int computeMetaStringSize(QObject *obj)
{
    const QMetaObject *meta = obj->metaObject();
    const uint *mdata = meta->d.data;
    char *msdata = const_cast<char *>(meta->d.stringdata);
    char *lastChar = const_cast<char *>(msdata);

    uint revision = mdata[0];
    if(revision != 6)
        return 0;

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

int computeMetaDataSize(QObject *obj)
{
    const QMetaObject *meta = obj->metaObject();
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

#include "qtcptransport.h"
#include <QHostAddress>

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);

	MocTest mtest;
	MocTest mtest2;
	//prepareForRemote(&mtest);

    qDebug() << computeMetaStringSize(&mtest);
    qDebug() << computeMetaDataSize(&mtest);

	//host = new QObjectHost();
	//QObjectHost::host->init();

    QMetaObject::invokeMethod(&mtest, "slot1");
	QObject::connect(&mtest, SIGNAL(stringChanged()), &mtest2, SLOT(slot1()));
	mtest.setString("LOLO");

    QTcpTransport transport(QHostAddress::Any, 6565);

	return a.exec();
}
