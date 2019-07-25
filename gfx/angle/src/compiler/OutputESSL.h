





#ifndef CROSSCOMPILERGLSL_OUTPUTESSL_H_
#define CROSSCOMPILERGLSL_OUTPUTESSL_H_

#include "compiler/OutputGLSLBase.h"

class TOutputESSL : public TOutputGLSLBase
{
public:
    TOutputESSL(TInfoSinkBase& objSink);

protected:
    virtual bool writeVariablePrecision(TPrecision precision);
};

#endif  
