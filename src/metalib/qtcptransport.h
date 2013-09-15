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
    QTcpTransport(QTcpServer *server, QObject *parent = 0);
	QTcpTransport(QTcpSocket *client, QObject *parent = 0);
    ~QTcpTransport();

    void broadcast(const QByteArray& data);
    //Replace raw pointer with some ID
    void write(QIODevice *client, const QByteArray& data);

private:
	void _addClient(QTcpSocket *client);
	void _removeClient(QTcpSocket *client);

private slots:
    void handleConnection();
    void handleDisconnection();
    void dispatch();
    QByteArray pack(const QByteArray& data);
    
private:
    typedef QList<QIODevice *> ClientList;
    QTcpServer *_server;
	QTcpSocket *_peer;
    ClientList _clients;
    
};

#endif // QTCPTRANSPORT_H
