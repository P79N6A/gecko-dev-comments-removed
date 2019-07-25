





#ifndef COMPILER_TRANSLATORGLSL_H_
#define COMPILER_TRANSLATORGLSL_H_

#include "compiler/ShHandle.h"

class TranslatorGLSL : public TCompiler {
public:
    TranslatorGLSL(ShShaderType type, ShShaderSpec spec);

protected:
    virtual void translate(TIntermNode* root);
};

#endif  
