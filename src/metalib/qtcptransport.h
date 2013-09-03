#ifndef QTCPTRANSPORT_H
#define QTCPTRANSPORT_H

#include "global.h"
#include "qmetatransport.h"

#include <QList>

class QTcpServer;
class QTcpSocket;
class QHostAddress;
class QIODevice;

class METAEXPORT QTcpTransport : public QObject
{
    Q_OBJECT

public:
    QTcpTransport(const QHostAddress& address, quint32 port, QObject *parent = 0);
    ~QTcpTransport();

    void broadcast(const QByteArray& data);
    //Replace raw pointer with some ID
    void write(QIODevice *client, const QByteArray& data);

private slots:
    void handleConnection();
    void handleDisconnection();
    void dispatch();
    
private:
    typedef QList<QIODevice *> ClientList;
    QTcpServer *_server;
    ClientList _clients;
    
};

#endif // QTCPTRANSPORT_H
