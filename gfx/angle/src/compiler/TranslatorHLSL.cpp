





#include "compiler/TranslatorHLSL.h"

#include "compiler/InitializeParseContext.h"
#include "compiler/OutputHLSL.h"

TranslatorHLSL::TranslatorHLSL(ShShaderType type, ShShaderSpec spec, ShShaderOutput output)
    : TCompiler(type, spec), mOutputType(output)
{
}

void TranslatorHLSL::translate(TIntermNode *root)
{
    TParseContext& parseContext = *GetGlobalParseContext();
    sh::OutputHLSL outputHLSL(parseContext, getResources(), mOutputType);

    outputHLSL.output();
    mActiveUniforms = outputHLSL.getUniforms();
}
