#include <QCoreApplication>
#include <QtNetwork/QTcpSocket>
#include <QHostAddress>

QByteArray pack(const QByteArray& data)
{
    QByteArray packet;
    QDataStream out(&packet, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_8);
    out << quint16(0);
    out << data;
    out.device()->seek(0);
    out << (quint16)(packet.size() - sizeof(quint16));

    return packet;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QTcpSocket client;
    client.connectToHost(QHostAddress::LocalHost, 6565, QIODevice::ReadWrite);

    QByteArray data;
    QDataStream outData(&data, QIODevice::WriteOnly);
    outData.setVersion(QDataStream::Qt_4_8);

    outData << quint32(0x01);
    outData << QString("MocTest1");

    QByteArray packet = pack(data);    

    client.write(packet);

    return a.exec();
}
