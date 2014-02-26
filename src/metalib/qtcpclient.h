#ifndef QTCPCLIENT_H
#define QTCPCLIENT_H

#include "metaglobal.h"
#include "qmetaclient.h"

#include <QAbstractSocket>
class QTcpSocket;

class METAEXPORT QTcpClient : public QMetaClient
{
    Q_OBJECT

public:
    QTcpClient(QMetaHost *host, QObject *parent = 0);
    ~QTcpClient();

    void setSocket(int descriptor);

};

#endif // QTCPCLIENT_H
