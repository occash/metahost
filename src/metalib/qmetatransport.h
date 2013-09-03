#ifndef QMETATRANSPORT_H
#define QMETATRANSPORT_H

#include <QObject>
#include <QIODevice>

class QMetaTransport : public QObject
{
    Q_OBJECT

public:
    QMetaTransport(QObject *parent = 0) : QObject(parent) {}
    ~QMetaTransport() = 0 {}

signals:
    void newClient(QIODevice *);
    void removeClient(QIODevice *);

};

#endif // QMETATRANSPORT_H
