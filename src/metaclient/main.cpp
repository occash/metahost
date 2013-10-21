#include <QApplication>
#include <QTcpSocket>

#include "clientwidget.h"
#include <proto.h>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    /*ClientWidget w;
	w.show();*/

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

    QTcpSocket client;
    client.connectToHost("127.0.0.1", 6565, QIODevice::ReadWrite);
    client.write(ptr, msglen);

    return a.exec();
}
