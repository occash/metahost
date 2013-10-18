#include "qmetaevent.h"

static int MetaEventType = QEvent::registerEventType();

QMetaEvent::QMetaEvent(char *data, quint16 size)
    : QEvent((QEvent::Type)MetaEventType),
    _data(data),
    _size(size)
{
}
