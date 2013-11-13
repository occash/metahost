#include "zeroconf.h"

#include <QDataStream>
#include <QNetworkInterface>

#include <iostream>

QZeroConfiguration::QZeroConfiguration(QObject *parent)
    : QObject(parent),
    _listen(false)
{
    listInterfaces();
    connect(&_socket, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
    bind();
}

QZeroConfiguration::~QZeroConfiguration()
{

}

bool QZeroConfiguration::lookup(const QString& service)
{
    QByteArray datagram;
    QDataStream stream(&datagram, QIODevice::WriteOnly);
    stream << (quint8)HasServers;
    stream << service;

    bool ok = true;
    foreach(const QHostAddress& address, _addresses)
    {
        qDebug() << "Broadcasting to" << address;
        int bytes = _socket.writeDatagram(datagram, 
            address, 6566);

        qDebug() << "Result:" << ((bytes != -1) ? "ok" : "bad");
        ok &= bytes != -1;
    }
    
    return ok;
}

void QZeroConfiguration::addService(const QString& service)
{
    if(_services.contains(service))
        return;

    _services.append(service);
}

void QZeroConfiguration::listen()
{
    _listen = true;
}

bool QZeroConfiguration::bind()
{
    if(_socket.state() != QAbstractSocket::BoundState) {
        bool res = _socket.bind(6566, QUdpSocket::DontShareAddress);
        qDebug() << (res ? "Bound" : "Not bound");
        return res;
    }

    qDebug() << "Already bound";
    return true;
}

void QZeroConfiguration::onReadyRead()
{
    qDebug() << "Ready read";
    while(_socket.hasPendingDatagrams()) 
    {
        QByteArray datagram;
        datagram.resize(_socket.pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;
        
        _socket.readDatagram(datagram.data(), datagram.size(),
            &sender, &senderPort);
        qDebug() << "Client at address" << sender << "and port" << senderPort;

        dispatchMessage(sender, senderPort, datagram);
    }
}

void QZeroConfiguration::dispatchMessage(const QHostAddress& address, quint16 port, 
    const QByteArray& datagram)
{
    QDataStream stream(datagram);
    quint8 msg = -1;
    stream >> msg;
    QString service;
    stream >> service;

    switch(msg)
    {
    case HasServers:
        if(_listen)
        {
            if(_services.indexOf(service) != -1)
            {
                std::cout << "Server has service: " << qPrintable(service) << std::endl;

                QByteArray answer;
                QDataStream stream(&answer, QIODevice::WriteOnly);
                stream << (quint8)IAmServer;
                stream << service;

                int bytes = _socket.writeDatagram(answer, address, 6566);
                qDebug() << "Answer" << ((bytes != -1) ? "sent" : "not sent");

                break;
            }
        }
        break;
    case IAmServer: 
    {
        std::cout << "Service found: " << qPrintable(service) << std::endl;
        emit newService(service, address);
        break;
    }
    default:
        break;
    }
}

void QZeroConfiguration::listInterfaces()
{
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();

    foreach(const QNetworkInterface& interface, interfaces){
        QList<QNetworkAddressEntry> addresses = interface.addressEntries();
        foreach(const QNetworkAddressEntry& address, addresses){
            QHostAddress broadcast = address.broadcast();
            if (!broadcast.isNull())
                _addresses.append(broadcast);
        }
    }

    if (_addresses.isEmpty())
        _addresses.append(QHostAddress::Broadcast);
}
