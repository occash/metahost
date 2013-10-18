#ifndef ENUMS_H
#define ENUMS_H

#include <QEvent>

enum RpcEvent {
    Input = QEvent::User + 1,
    Max = QEvent::MaxUser
};

#endif //ENUMS_H