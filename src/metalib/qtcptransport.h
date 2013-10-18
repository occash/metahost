#ifndef QTCPTRANSPORT_H
#define QTCPTRANSPORT_H

#include "global.h"
#include "qmetatransport.h"

#include <QList>

class QTcpServer;
class QTcpSocket;
class QHostAddress;
class QIODevice;
class QMetaClient;
class QMetaHost;

class METAEXPORT QTcpTransport : public QObject
{
    Q_OBJECT

public:
    QTcpTransport(QMetaHost *host, QTcpServer *server, QObject *parent = 0);
	QTcpTransport(QMetaHost *host, QTcpSocket *client, QObject *parent = 0);
    ~QTcpTransport();

private:
	void _addClient(QTcpSocket *client);
	void _removeClient(QTcpSocket *client);

private slots:
    void handleConnection();
    void handleDisconnection();
    
private:
    QMetaHost *_host;
    typedef QList<QMetaClient *> ClientList;
    QTcpServer *_server;
	QTcpSocket *_peer;
    ClientList _clients;
    
};

#endif // QTCPTRANSPORT_H
