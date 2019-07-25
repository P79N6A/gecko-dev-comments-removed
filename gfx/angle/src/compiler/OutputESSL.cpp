





#include "compiler/OutputESSL.h"

TOutputESSL::TOutputESSL(TInfoSinkBase& objSink)
    : TOutputGLSLBase(objSink)
{
}

bool TOutputESSL::writeVariablePrecision(TPrecision precision)
{
    if (precision == EbpUndefined)
        return false;

    TInfoSinkBase& out = objSink();
    out << getPrecisionString(precision);
    return true;
}
