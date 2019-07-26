





#ifndef COMPILER_TRANSLATORHLSL_H_
#define COMPILER_TRANSLATORHLSL_H_

#include "compiler/ShHandle.h"
#include "compiler/CompilerUniform.h"

class TranslatorHLSL : public TCompiler {
public:
    TranslatorHLSL(ShShaderType type, ShShaderSpec spec, ShShaderOutput output);

    virtual TranslatorHLSL *getAsTranslatorHLSL() { return this; }
    const sh::ActiveUniforms &getUniforms() { return mActiveUniforms; }

protected:
    virtual void translate(TIntermNode* root);

    sh::ActiveUniforms mActiveUniforms;
    ShShaderOutput mOutputType;
};

#endif  
