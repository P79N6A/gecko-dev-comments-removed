





#include "compiler/OutputGLSL.h"

TOutputGLSL::TOutputGLSL(TInfoSinkBase& objSink,
                         ShHashFunction64 hashFunction,
                         NameMap& nameMap,
                         TSymbolTable& symbolTable)
    : TOutputGLSLBase(objSink, hashFunction, nameMap, symbolTable)
{
}

bool TOutputGLSL::writeVariablePrecision(TPrecision)
{
    return false;
}
