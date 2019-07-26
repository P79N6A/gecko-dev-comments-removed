





#include "compiler/TranslatorHLSL.h"






TCompiler* ConstructCompiler(
    ShShaderType type, ShShaderSpec spec, ShShaderOutput output)
{
  switch (output)
  {
    case SH_HLSL9_OUTPUT:
    case SH_HLSL11_OUTPUT:
      return new TranslatorHLSL(type, spec, output);
    default:
      return NULL;
  }
}




void DeleteCompiler(TCompiler* compiler)
{
    delete compiler;
}
