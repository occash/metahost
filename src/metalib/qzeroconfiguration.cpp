#include "qzeroconfiguration.h"

#include <QDataStream>
#include <QNetworkInterface>

QZeroService::QZeroService(const QString &name, QObject *parent)
    : QObject(parent),
      _name(name)
{
}

QZeroService::QZeroService(const QZeroService &other)
{
    _name = other._name;
    _fields = other._fields;
}

QZeroService QZeroService::operator =(const QZeroService &other)
{
    _name = other._name;
    _fields = other._fields;
    return *this;
}

QZeroService::~QZeroService()
{
}

QString QZeroService::name() const
{
    return _name;
}

void QZeroService::setName(const QString &name)
{
    _name = name;
}

void QZeroService::insertField(const QString &key, const QVariant &value)
{
    _fields.insert(key, value);
}

void QZeroService::removeField(const QString &key)
{
    _fields.remove(key);
}

const QVariant QZeroService::field(const QString &key) const
{
    return _fields.value(key);
}

QStringList QZeroService::fields() const
{
    return _fields.keys();
}

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
        int bytes = _socket.writeDatagram(datagram, 
            address, 6566);

        ok &= bytes != -1;
    }
    
    return ok;
}

bool QZeroConfiguration::claim(const QString &service, const QHostAddress &host)
{
    QByteArray datagram;
    QDataStream stream(&datagram, QIODevice::WriteOnly);
    stream << (quint8)HasServers;
    stream << service;

    bool ok = true;
    int bytes = _socket.writeDatagram(datagram,
        host, 6566);

    ok &= bytes != -1;

    return ok;
}

void QZeroConfiguration::addService(const QZeroService& service)
{
    if(_services.contains(service.name()))
        return;

    _services.insert(service.name(), service);
}

void QZeroConfiguration::listen()
{
    _listen = true;
}

bool QZeroConfiguration::bind()
{
    if(_socket.state() != QAbstractSocket::BoundState) {
        bool res = _socket.bind(6566, QUdpSocket::DontShareAddress);
        return res;
    }

    return true;
}

void QZeroConfiguration::onReadyRead()
{
    while(_socket.hasPendingDatagrams()) 
    {
        QByteArray datagram;
        datagram.resize(_socket.pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;
        
        _socket.readDatagram(datagram.data(), datagram.size(),
            &sender, &senderPort);

        dispatchMessage(sender, senderPort, datagram);
    }
}

void QZeroConfiguration::dispatchMessage(const QHostAddress& address, quint16 port, 
    const QByteArray& datagram)
{
    QDataStream in(datagram);
    quint8 msg = -1;
    in >> msg;
    QString name;
    in >> name;

    switch(msg)
    {
    case HasServers:
        if(_listen)
        {
            if(_services.contains(name))
            {
                QByteArray answer;
                QDataStream out(&answer, QIODevice::WriteOnly);

                QZeroService service = _services.value(name);
                QStringList fields = service.fields();
                int fsize = fields.size();

                out << (quint8)IAmServer;
                out << name;
                out << fsize;

                foreach(const QString& field, fields)
                {
                    out << field;
                    out << service.field(field);
                }

                _socket.writeDatagram(answer, address, 6566);

                break;
            }
        }
        break;
    case IAmServer: 
    {
        QZeroService service(name);
        int fsize = 0;
        in >> fsize;
        for(int i = 0; i < fsize; ++i)
        {
            QString field;
            QVariant value;
            in >> field;
            in >> value;
            service.insertField(field, value);
        }

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
