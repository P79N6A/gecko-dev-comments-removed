





#ifndef COMPILER_TRANSLATORESSL_H_
#define COMPILER_TRANSLATORESSL_H_

#include "compiler/ShHandle.h"

class TranslatorESSL : public TCompiler {
public:
    TranslatorESSL(ShShaderType type, ShShaderSpec spec);

protected:
    virtual void translate(TIntermNode* root);

private:
    void writeExtensionBehavior();
};

#endif  
