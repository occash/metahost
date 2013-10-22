#ifndef RPCEVENT_H
#define RPCEVENT_H

#include "enums.h"
#include <QEvent>

extern int MetaEventType;

class QMetaEvent : public QEvent
{
public:
    QMetaEvent(QObject *container, char *data);

    QObject *container() { return _container; }
    char *data() { return _data; }

private:
    QObject *_container;
    char *_data;

};

#endif //RPCEVENT_H