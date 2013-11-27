#ifndef ZEROCONF_H
#define ZEROCONF_H

#include <QObject>
#include <QUdpSocket>
#include <QStringList>
#include <QMap>
#include <QVariant>
#include "global.h"

class METAEXPORT QZeroService : public QObject
{
    Q_OBJECT

public:
    QZeroService(const QString& name = QString(),
                 QObject *parent = 0);
    QZeroService(const QZeroService& other);
    QZeroService operator=(const QZeroService& other);
    ~QZeroService();

    QString name() const;
    void setName(const QString& name);
    void insertField(const QString& key,
                     const QVariant& value);
    void removeField(const QString& key);
    const QVariant field(const QString& key) const;
    QStringList fields() const;

private:
    typedef QMap<QString, QVariant> FieldMap;
    QString _name;
    FieldMap _fields;

};

class METAEXPORT QZeroConfiguration : public QObject
{
    Q_OBJECT

public:
    QZeroConfiguration(QObject *parent = 0);
    ~QZeroConfiguration();

    bool lookup(const QString& service);
    bool claim(const QString& service, const QHostAddress& host);

    void addService(const QZeroService& service);
    void listen();

signals:
    void newService(const QZeroService& service, const QHostAddress& address);

private slots:
    void onReadyRead();

private:
    bool bind();
    void dispatchMessage(const QHostAddress& address, quint16 port, 
        const QByteArray& datagram);
    void listInterfaces();

private:
    enum MessageType {
        HasServers = 0x01,
        IAmServer = 0x02
    };
    QUdpSocket _socket;
    QMap<QString, QZeroService> _services;
    QList<QHostAddress> _addresses;
    bool _listen;

};

#endif // ZEROCONF_H
