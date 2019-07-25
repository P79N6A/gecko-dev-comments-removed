





#include "compiler/OutputGLSL.h"

TOutputGLSL::TOutputGLSL(TInfoSinkBase& objSink)
    : TOutputGLSLBase(objSink)
{
}

bool TOutputGLSL::writeVariablePrecision(TPrecision)
{
    return false;
}
