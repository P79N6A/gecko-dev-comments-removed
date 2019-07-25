





#include "compiler/TranslatorHLSL.h"






TCompiler* ConstructCompiler(EShLanguage language, int debugOptions)
{
    return new TranslatorHLSL(language, debugOptions);
}




void DeleteCompiler(TCompiler* compiler)
{
    delete compiler;
}
