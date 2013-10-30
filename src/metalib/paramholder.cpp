#include "paramholder.h"

ParamHolder::ParamHolder() :
    QObject(nullptr),
    timeout(false)
{
    param.returnId = 1;
}

void ParamHolder::setParam(const ReturnParam &param)
{
    this->param = param;
}

void ParamHolder::setTimeout()
{
    timeout = true;
}
