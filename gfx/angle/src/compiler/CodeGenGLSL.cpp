





#include "compiler/TranslatorGLSL.h"






TCompiler* ConstructCompiler(EShLanguage language, int debugOptions)
{
    return new TranslatorGLSL(language, debugOptions);
}




void DeleteCompiler(TCompiler* compiler)
{
    delete compiler;
}
