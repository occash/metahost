# MetaHost
## Introduction
MetaHost is simple Qt library for remote metamethod calls, signal/slot connections and property handling.

## Build and Install

To build MetaHost library run the following commands:
```sh
cd src
qmake
make
make install
```

## Usage
### Server
```c++
QTcpServer server;
server->listen(QHostAddress::Any, 6565);
QTcpTransport transport(&server);
QMetaHost host;

//*********************

QObject *someObject = new MyObject();
someObject->setName("MyObject1");
host.registerObject(someObject);
```

### Client
```c++
QTcpSocket socket;
socket->connectToHost(address, 6565);
QMetaClient transport;
transport.setDevice(&socket);
QMetaHost host;

//*********************

QObject *someObjet = host->getObject("MyObject1", &transport);
connect(someObject, SIGNAL(signal1()), ...);
```
