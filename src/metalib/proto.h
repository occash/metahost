#ifndef PROTO_H
#define PROTO_H

namespace meta {
namespace proto {
enum msg {
    revision, //negotiate qt meta system revision
    metainfo, //request object metainfo
    metacall, //call object meta method
    emitsignal, //emit object signal
    property //read/write object poperties
};
}
}

#endif // PROTO_H
