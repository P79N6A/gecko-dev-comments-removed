





#include "compiler/TranslatorHLSL.h"






TCompiler* ConstructCompiler(
    ShShaderType type, ShShaderSpec spec, ShShaderOutput output)
{
  switch (output) {
    case SH_HLSL_OUTPUT:
      return new TranslatorHLSL(type, spec);
    default:
      return NULL;
  }
}




void DeleteCompiler(TCompiler* compiler)
{
    delete compiler;
}
