





#include "compiler/translator/OutputESSL.h"

TOutputESSL::TOutputESSL(TInfoSinkBase& objSink,
                         ShArrayIndexClampingStrategy clampingStrategy,
                         ShHashFunction64 hashFunction,
                         NameMap& nameMap,
                         TSymbolTable& symbolTable,
                         int shaderVersion)
    : TOutputGLSLBase(objSink, clampingStrategy, hashFunction, nameMap, symbolTable, shaderVersion)
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
