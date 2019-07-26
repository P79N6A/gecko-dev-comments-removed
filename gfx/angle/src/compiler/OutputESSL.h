





#ifndef CROSSCOMPILERGLSL_OUTPUTESSL_H_
#define CROSSCOMPILERGLSL_OUTPUTESSL_H_

#include "compiler/OutputGLSLBase.h"

class TOutputESSL : public TOutputGLSLBase
{
public:
    TOutputESSL(TInfoSinkBase& objSink,
                ShHashFunction64 hashFunction,
                NameMap& nameMap,
                TSymbolTable& symbolTable);

protected:
    virtual bool writeVariablePrecision(TPrecision precision);
};

#endif  
