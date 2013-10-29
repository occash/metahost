#ifndef PARAMHOLDER_H
#define PARAMHOLDER_H

#include <QObject>

struct ReturnParam {
    QMetaObject::Call callType;
    int methodIndex;
    int returnId;
    QByteArray returnArg;
};

class ParamHolder : public QObject
{
    Q_OBJECT

public:
    ParamHolder();

    ReturnParam param;

public slots:
    void setParam(const ReturnParam& param);
};

#endif // PARAMHOLDER_H
