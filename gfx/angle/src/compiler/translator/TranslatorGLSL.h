





#ifndef COMPILER_TRANSLATORGLSL_H_
#define COMPILER_TRANSLATORGLSL_H_

#include "compiler/translator/Compiler.h"

class TranslatorGLSL : public TCompiler {
public:
    TranslatorGLSL(sh::GLenum type, ShShaderSpec spec);

protected:
    virtual void translate(TIntermNode* root);

private:
    void writeExtensionBehavior();
};

#endif  
