





#ifndef COMPILER_TRANSLATORHLSL_H_
#define COMPILER_TRANSLATORHLSL_H_

#include "compiler/ShHandle.h"

class TranslatorHLSL : public TCompiler {
public:
    TranslatorHLSL(EShLanguage l, int dOptions);
    virtual bool compile(TIntermNode* root);
    int debugOptions;
};

#endif  
