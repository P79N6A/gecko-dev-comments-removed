





#ifndef CROSSCOMPILERGLSL_OUTPUTGLSL_H_
#define CROSSCOMPILERGLSL_OUTPUTGLSL_H_

#include "compiler/OutputGLSLBase.h"

class TOutputGLSL : public TOutputGLSLBase
{
public:
    TOutputGLSL(TInfoSinkBase& objSink,
                ShHashFunction64 hashFunction,
                NameMap& nameMap,
                TSymbolTable& symbolTable);

protected:
    virtual bool writeVariablePrecision(TPrecision);
};

#endif  
