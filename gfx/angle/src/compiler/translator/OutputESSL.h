





#ifndef CROSSCOMPILERGLSL_OUTPUTESSL_H_
#define CROSSCOMPILERGLSL_OUTPUTESSL_H_

#include "compiler/translator/OutputGLSLBase.h"

class TOutputESSL : public TOutputGLSLBase
{
public:
    TOutputESSL(TInfoSinkBase& objSink,
                ShArrayIndexClampingStrategy clampingStrategy,
                ShHashFunction64 hashFunction,
                NameMap& nameMap,
                TSymbolTable& symbolTable,
                int shaderVersion);

protected:
    virtual bool writeVariablePrecision(TPrecision precision);
};

#endif  
