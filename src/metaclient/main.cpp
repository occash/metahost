#include <QCoreApplication>
#include <QtNetwork/QTcpSocket>
#include <QHostAddress>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QTcpSocket client;
    client.connectToHost(QHostAddress::LocalHost, 6565, QIODevice::ReadWrite);

    return a.exec();
}
