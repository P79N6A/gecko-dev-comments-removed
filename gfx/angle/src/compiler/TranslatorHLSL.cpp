





#include "compiler/TranslatorHLSL.h"

#include "compiler/OutputHLSL.h"

TranslatorHLSL::TranslatorHLSL(EShLanguage language, int debugOptions)
    : TCompiler(language), debugOptions(debugOptions)
{
}

bool TranslatorHLSL::compile(TIntermNode *root)
{
    TParseContext& parseContext = *GetGlobalParseContext();
    sh::OutputHLSL outputHLSL(parseContext);

    outputHLSL.output();
    
    return true;
}
