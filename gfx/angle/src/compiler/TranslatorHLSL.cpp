





#include "compiler/TranslatorHLSL.h"

#include "compiler/OutputHLSL.h"

TranslatorHLSL::TranslatorHLSL(ShShaderType type, ShShaderSpec spec)
    : TCompiler(type, spec)
{
}

void TranslatorHLSL::translate(TIntermNode *root)
{
    TParseContext& parseContext = *GetGlobalParseContext();
    sh::OutputHLSL outputHLSL(parseContext);

    outputHLSL.output();
}
