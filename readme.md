## MetaHost
# Introduction
MetaHost is simple Qt library for remote metamethod calls, signal/slot connections and property handling.

#Build and Install
For building MetaHost library just do:
```
cd src
qmake
make
make install
```

#Usage
*Server*
```
QTcpServer server;
server->listen(QHostAddress::Any, 6565);
QTcpTransport transport(&server);
QMetaHost host;


QObject *someObject = new MyObject();
someObject->setName("MyObject1");
host.registerObject(someObject);
```

*Client*
```
QTcpSocket socket;
socket->connectToHost(address, 6565);
QMetaClient transport;
transport.setDevice(&socket);
QMetaHost host;

...

QObject *someObjet = host->getObject("MyObject1", &transport);
connect(someObject, SIGNAL(signal1()), ...);
```