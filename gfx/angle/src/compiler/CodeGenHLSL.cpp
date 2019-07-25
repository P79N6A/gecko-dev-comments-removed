





#include "compiler/TranslatorHLSL.h"






TCompiler* ConstructCompiler(EShLanguage language, EShSpec spec)
{
    return new TranslatorHLSL(language, spec);
}




void DeleteCompiler(TCompiler* compiler)
{
    delete compiler;
}
