





#ifndef COMPILER_TRANSLATORESSL_H_
#define COMPILER_TRANSLATORESSL_H_

#include "compiler/translator/Compiler.h"

class TranslatorESSL : public TCompiler {
public:
    TranslatorESSL(ShShaderType type, ShShaderSpec spec);

protected:
    virtual void translate(TIntermNode* root);

private:
    void writeExtensionBehavior();
};

#endif  
