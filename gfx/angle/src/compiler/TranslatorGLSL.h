





#ifndef COMPILER_TRANSLATORGLSL_H_
#define COMPILER_TRANSLATORGLSL_H_

#include "compiler/ShHandle.h"

class TranslatorGLSL : public TCompiler {
public:
    TranslatorGLSL(EShLanguage l, int dOptions);
    virtual bool compile(TIntermNode* root);
    int debugOptions;
};

#endif  
