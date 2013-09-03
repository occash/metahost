#include <QCoreApplication>
#include <QtNetwork/QTcpSocket>
#include <QHostAddress>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QTcpSocket client;
    client.connectToHost(QHostAddress::LocalHost, 6565, QIODevice::ReadWrite);

    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);

    out << quint16(0);
    out << "MocTest";
    out.device()->seek(0);
    out << (quint16)(data.size() - sizeof(quint16));

    client.write(data);

    return a.exec();
}
