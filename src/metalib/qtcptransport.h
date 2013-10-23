#ifndef QTCPTRANSPORT_H
#define QTCPTRANSPORT_H

#include "global.h"

#include <QObject>
#include <QList>

class QTcpServer;
class QTcpSocket;
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
    typedef QList<QMetaClient *> ClientList;

    QMetaHost *_host;
    QTcpServer *_server;
	QTcpSocket *_peer;
    ClientList _clients;
    
};

#endif // QTCPTRANSPORT_H
