#include "qmetaclient.h"
#include "qmetaevent.h"
#include "qmetahost.h"

#include <QIODevice>
#include <QCoreApplication>

QMetaClient::QMetaClient(QMetaHost *host, QObject *parent) :
    QObject(parent),
    _host(host),
    _device(nullptr)
{
}

bool QMetaClient::event(QEvent *e)
{
    if(e->type() == MetaEventType)
    {
        QMetaEvent *me = static_cast<QMetaEvent *>(e);

        if(_device) {
            quint16 packetSize = *((quint16 *)me->data());
            _device->write(me->data(), packetSize + sizeof(quint16));
        }

        return true;
    }

    return false;
}

void QMetaClient::onReadyRead()
{
    while(_device->bytesAvailable())
    {
        quint16 packetSize = 0;
        char *input = reinterpret_cast<char *>(&packetSize);
        if(_device->read(input, sizeof(quint16)) == -1)
            break;

        //TODO: read partially
        if(!packetSize || packetSize > _device->bytesAvailable())
            return;

        char *data = new char[packetSize];
        if(_device->read(data, packetSize) == packetSize)
        {
            QMetaEvent *event = new QMetaEvent(this, data);
            QCoreApplication::postEvent(_host, event, Qt::HighEventPriority);
        }
    }
}

void QMetaClient::setDevice(QIODevice *d)
{
    _device = d;
    _device->setParent(this);
    connect(_device, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
}

QIODevice * QMetaClient::device() const
{
    return _device;
}
