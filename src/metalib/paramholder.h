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
    bool timeout;

public slots:
    void setParam(const ReturnParam& param);
    void setTimeout();
};

#endif // PARAMHOLDER_H
