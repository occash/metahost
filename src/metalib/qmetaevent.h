#ifndef RPCEVENT_H
#define RPCEVENT_H

#include "enums.h"
#include <QEvent>

extern int MetaEventType;

class QMetaEvent : public QEvent
{
public:
    QMetaEvent(char *data, quint16 size);

    char *data() { return _data; }
    quint16 size() { return _size; }

private:
    char *_data;
    quint16 _size;

};

#endif //RPCEVENT_H