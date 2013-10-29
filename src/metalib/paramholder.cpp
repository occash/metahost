#include "paramholder.h"

ParamHolder::ParamHolder() :
    QObject(nullptr)
{
}

void ParamHolder::setParam(const ReturnParam &param)
{
    this->param = param;
}
