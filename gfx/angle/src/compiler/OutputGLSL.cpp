





#include "compiler/OutputGLSL.h"

TOutputGLSL::TOutputGLSL(TInfoSinkBase& objSink,
                         ShArrayIndexClampingStrategy clampingStrategy,
                         ShHashFunction64 hashFunction,
                         NameMap& nameMap,
                         TSymbolTable& symbolTable)
    : TOutputGLSLBase(objSink, clampingStrategy, hashFunction, nameMap, symbolTable)
{
}

bool TOutputGLSL::writeVariablePrecision(TPrecision)
{
    return false;
}
