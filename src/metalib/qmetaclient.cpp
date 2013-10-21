#include "qmetaclient.h"
#include "qmetaevent.h"
#include "qmetahost.h"

#include <QIODevice>
#include <QCoreApplication>

QMetaClient::QMetaClient(QMetaHost *host, QObject *parent) :
    QObject(parent),
    _host(host),
    _client(nullptr)
{
}

bool QMetaClient::event(QEvent *e)
{
    if(e->type() == MetaEventType)
    {
        QMetaEvent *me = static_cast<QMetaEvent *>(e);

        if(_client) {
            quint16 packetSize = me->size();
            _client->write(reinterpret_cast<char *>(&packetSize),
                sizeof(quint16));
            _client->write(me->data(), me->size());
        }

        return true;
    }

    return false;
}

void QMetaClient::onReadyRead()
{
    while(_client->bytesAvailable())
    {
        quint16 packetSize = 0;
        char *input = reinterpret_cast<char *>(&packetSize);
        if(_client->read(input, sizeof(quint16)) == -1)
            break;

        int bytes = _client->bytesAvailable();
        if(!packetSize || packetSize > _client->bytesAvailable())
            return;

        char *data = new char[packetSize];
        if(_client->read(data, packetSize) == packetSize)
        {
            QMetaEvent *event = new QMetaEvent(
                data, packetSize);
            QCoreApplication::postEvent(_host, event, Qt::HighEventPriority);
        }
    }
}

void QMetaClient::setDevice(QIODevice *d)
{
    _client = d;
    _client->setParent(this);
    connect(_client, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
}

/*QByteArray QMetaClient::pack(const QByteArray& data)
{
    QByteArray packet;
    QDataStream out(&packet, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_8);
    out << quint16(0);
    out << data;
    out.device()->seek(0);
    out << (quint16)(packet.size() - sizeof(quint16));

    return packet;
}*/