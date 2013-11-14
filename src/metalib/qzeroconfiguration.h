#ifndef ZEROCONF_H
#define ZEROCONF_H

#include <QObject>
#include <QUdpSocket>
#include <QStringList>
#include "global.h"

class METAEXPORT QZeroConfiguration : public QObject
{
    Q_OBJECT

public:
    QZeroConfiguration(QObject *parent = 0);
    ~QZeroConfiguration();

    bool lookup(const QString& service);
    void addService(const QString& service);
    void listen();

signals:
    void newService(const QString& service, const QHostAddress& address);

private slots:
    void onReadyRead();

private:
    bool bind();
    void dispatchMessage(const QHostAddress& address, quint16 port, 
        const QByteArray& datagram);
    void listInterfaces();

private:
    enum MessageType {
        HasServers = 0x01,
        IAmServer = 0x02
    };
    QUdpSocket _socket;
    QStringList _services;
    QList<QHostAddress> _addresses;
    bool _listen;

};

#endif // ZEROCONF_H
