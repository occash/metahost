#ifndef QMETACLIENT_H
#define QMETACLIENT_H

#include <QObject>

class QIODevice;
class QMetaHost;

class QMetaClient : public QObject
{
    Q_OBJECT
public:
    QMetaClient(QMetaHost *host, QObject *parent = 0);

    void setDevice(QIODevice *d);

protected:
    bool event(QEvent *e);
    
private slots:
    void onReadyRead();

/*private:
    QByteArray pack(const QByteArray&);*/

private:
    QMetaHost *_host;
    QIODevice *_client;
    
};

#endif // QMETACLIENT_H
