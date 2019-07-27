





#include "compiler/translator/TranslatorESSL.h"
#include "compiler/translator/TranslatorGLSL.h"
#include "compiler/translator/TranslatorHLSL.h"






TCompiler* ConstructCompiler(
    sh::GLenum type, ShShaderSpec spec, ShShaderOutput output)
{
    switch (output) {
    case SH_ESSL_OUTPUT:
        return new TranslatorESSL(type, spec);
    case SH_GLSL_OUTPUT:
        return new TranslatorGLSL(type, spec);
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
