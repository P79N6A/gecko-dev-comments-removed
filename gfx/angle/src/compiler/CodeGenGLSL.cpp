





#include "compiler/TranslatorGLSL.h"
#include "compiler/TranslatorESSL.h"






TCompiler* ConstructCompiler(
    ShShaderType type, ShShaderSpec spec, ShShaderOutput output)
{
    switch (output) {
      case SH_GLSL_OUTPUT:
        return new TranslatorGLSL(type, spec);
      case SH_ESSL_OUTPUT:
        return new TranslatorESSL(type, spec);
      default:
        return NULL;
    }
}




void DeleteCompiler(TCompiler* compiler)
{
    delete compiler;
}
