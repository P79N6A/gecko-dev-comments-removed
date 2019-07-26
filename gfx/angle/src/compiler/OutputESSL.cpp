





#include "compiler/OutputESSL.h"

TOutputESSL::TOutputESSL(TInfoSinkBase& objSink,
                         ShHashFunction64 hashFunction,
                         NameMap& nameMap,
                         TSymbolTable& symbolTable)
    : TOutputGLSLBase(objSink, hashFunction, nameMap, symbolTable)
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
