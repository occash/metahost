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
            quint16 packetSize = *((quint16 *)me->data());
            _client->write(me->data(), packetSize + sizeof(quint16));
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

        //TODO: read partially
        if(!packetSize || packetSize > _client->bytesAvailable())
            return;

        char *data = new char[packetSize];
        if(_client->read(data, packetSize) == packetSize)
        {
            QMetaEvent *event = new QMetaEvent(this, data);
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