#include "qmetaevent.h"

int MetaEventType = QEvent::registerEventType();

QMetaEvent::QMetaEvent(QObject *container, char *data)
    : QEvent((QEvent::Type)MetaEventType),
    _container(container),
    _data(data)
{
}
