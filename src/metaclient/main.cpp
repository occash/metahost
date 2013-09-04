#include <QCoreApplication>
#include <QtNetwork/QTcpSocket>
#include <QHostAddress>
#include <QFile>
#include <QStringList>

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

    client.waitForReadyRead(50000);
    
    QDataStream in(&client);
    in.setVersion(QDataStream::Qt_4_8);

    quint16 size = 0;
    in >> size;
    qDebug() << "Bytes available" << client.bytesAvailable();
    qDebug() << "Data expected:" << size;

    if(client.bytesAvailable() < size)
    {
        //Make appropriate logging
        qDebug() << "Error: Number of bytes manipulated!";
    }

    QByteArray datain;
    in >> datain;
    qDebug() << "Data received:" << datain;
    qDebug() << datain.size();

    //unpack
    QDataStream instream(&datain, QIODevice::ReadOnly);
    instream.setVersion(QDataStream::Qt_4_8);
    int sz = 0;
    instream >> sz;

    QStringList classNames;

    for(int i = 0; i < sz; ++i)
    {
        QString cname;
        instream >> cname;
        qDebug() << cname;
    }

    return a.exec();
}
