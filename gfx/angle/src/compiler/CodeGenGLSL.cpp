





#include "compiler/TranslatorGLSL.h"






TCompiler* ConstructCompiler(ShShaderType type, ShShaderSpec spec)
{
    return new TranslatorGLSL(type, spec);
}




void DeleteCompiler(TCompiler* compiler)
{
    delete compiler;
}
