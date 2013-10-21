#include <QApplication>
#include <QTcpSocket>
#include <QPushButton>
#include <QMetaObject>

#include "clientwidget.h"
#include <proto.h>

void testQueryObjectInfo(char **data, qint64 *size)
{
    //Send query object message
    int szlen = sizeof(quint16); //size of packet
    int tplen = sizeof(quint8); //size of type in header
    char *objName = "ServerButton";
    int nlen = strlen(objName) + 1;
    quint64 msglen = szlen + tplen + nlen;
    quint16 packlen = tplen + nlen;

    char *msg = new char[msglen];
    char *ptr = msg;

    //Copy data to msg
    *((quint16 *)msg) = packlen;
    msg += szlen;
    *((quint8 *)msg) = QueryObjectInfo;
    msg += tplen;
    memcpy(msg, objName, nlen);

    *data = ptr;
    *size = msglen;
}

void testReturnObjectInfo(char **data, qint64 *size)
{
    quint32 id = 1;

    char *classNames[4] = {
        "QPushButton",
        "QAbstractButton",
        "QWidget",
        "QObject"
    };

    int answerSize = sizeof(quint8) + sizeof(quint32) + 1; 
    for(int i = 0; i < 4; ++i)
        answerSize += strlen(classNames[i]) + 1;


    //Allocate memory for answer
    int headerSize = sizeof(quint16);
    *data = new char[headerSize + answerSize];
    *size = headerSize + answerSize;
    char *ptr = *data;

    //Set header size
    *((quint16 *)ptr) = answerSize;
    ptr += sizeof(quint16);

    //Set header type
    quint8 type = ReturnObjectInfo;
    *((quint8 *)ptr) = type;
    ptr += sizeof(quint8);

    //Set generated object id
    *((quint32 *)ptr) = id;
    ptr += sizeof(quint32);

    //Set class names related to object
    for(int i = 0; i < 4; ++i)
    {
        int len = strlen(classNames[i]) + 1;
        memcpy(ptr,classNames[i], len);
        ptr += len;
    }

    //Add terminating zero
    *ptr = '\0';
}

void testQueryClassInfo(char **data, qint64 *size)
{
    //Send query object message
    int szlen = sizeof(quint16); //size of packet
    int tplen = sizeof(quint8); //size of type in header
    char *clsName = "QPushButton";
    int nlen = strlen(clsName) + 1;
    quint64 msglen = szlen + tplen + nlen;
    quint16 packlen = tplen + nlen;

    char *msg = new char[msglen];
    char *ptr = msg;

    //Copy data to msg
    *((quint16 *)msg) = packlen;
    msg += szlen;
    *((quint8 *)msg) = QueryClassInfo;
    msg += tplen;
    memcpy(msg, clsName, nlen);

    *data = ptr;
    *size = msglen;
}

void testReturnClassInfo(char **data, qint64 *size)
{
    const QMetaObject *meta = &QPushButton::staticMetaObject;
    char *className = "QPushButton";
    quint16 dataSize = 72;
    quint16 stringSize = 37;

    int answerSize = sizeof(quint8) + 1;
    answerSize += strlen(className);
    answerSize += sizeof(quint32) * 2;
    answerSize += dataSize;
    answerSize += stringSize;

    //Allocate memory for answer
    int headerSize = sizeof(quint16);
    *data = new char[headerSize + answerSize];
    *size = headerSize + answerSize;
    char *ptr = *data;

    //Set header size
    *((quint16 *)ptr) = answerSize;
    ptr += sizeof(quint16);

    //Set header type
    quint8 type = ReturnClassInfo;
    *((quint8 *)ptr) = type;
    ptr += sizeof(quint8);

    memcpy(ptr, className, strlen(className) + 1);
    ptr += strlen(className) + 1;

    *((quint16 *)ptr) = dataSize;
    ptr += sizeof(quint16);

    memcpy(ptr, meta->d.data, dataSize);
    ptr += dataSize;

    *((quint16 *)ptr) = stringSize;
    ptr += sizeof(quint16);

    memcpy(ptr, meta->d.stringdata, stringSize);
    ptr += stringSize;
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    /*ClientWidget w;
	w.show();*/

    QTcpSocket client;
    client.connectToHost("127.0.0.1", 6565, QIODevice::ReadWrite);
    char *ptr = nullptr;
    qint64 msglen = 0;

    /*testQueryObjectInfo(&ptr, &msglen);
    client.write(ptr, msglen);
    delete ptr;*/

    /*testReturnObjectInfo(&ptr, &msglen);
    client.write(ptr, msglen);
    delete ptr;*/

    /*testQueryClassInfo(&ptr, &msglen);
    client.write(ptr, msglen);
    delete ptr;*/

    testReturnClassInfo(&ptr, &msglen);
    client.write(ptr, msglen);
    delete ptr;

    return a.exec();
}
