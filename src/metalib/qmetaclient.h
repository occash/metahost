#ifndef QMETACLIENT_H
#define QMETACLIENT_H

#include <QObject>
#include "global.h"

class QIODevice;
class QMetaHost;

class METAEXPORT QMetaClient : public QObject
{
    Q_OBJECT
public:
    QMetaClient(QMetaHost *host, QObject *parent = 0);
    void setDevice(QIODevice *d);

protected:
    bool event(QEvent *e);
    
private slots:
    void onReadyRead();

private:
    QMetaHost *_host;
    QIODevice *_client;
    
};

#endif // QMETACLIENT_H
