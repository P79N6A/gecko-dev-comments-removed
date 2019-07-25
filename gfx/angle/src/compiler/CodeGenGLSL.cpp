





#include "compiler/TranslatorGLSL.h"






TCompiler* ConstructCompiler(EShLanguage language, EShSpec spec)
{
    return new TranslatorGLSL(language, spec);
}




void DeleteCompiler(TCompiler* compiler)
{
    delete compiler;
}
