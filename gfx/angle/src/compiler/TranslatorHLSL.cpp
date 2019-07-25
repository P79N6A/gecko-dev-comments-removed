





#include "compiler/TranslatorHLSL.h"

#include "compiler/OutputHLSL.h"

TranslatorHLSL::TranslatorHLSL(EShLanguage lang, EShSpec spec)
    : TCompiler(lang, spec)
{
}

bool TranslatorHLSL::compile(TIntermNode *root)
{
    TParseContext& parseContext = *GetGlobalParseContext();
    sh::OutputHLSL outputHLSL(parseContext);

    outputHLSL.output();
    
    return true;
}
