





#include "compiler/TranslatorHLSL.h"






TCompiler* ConstructCompiler(ShShaderType type, ShShaderSpec spec)
{
    return new TranslatorHLSL(type, spec);
}




void DeleteCompiler(TCompiler* compiler)
{
    delete compiler;
}
