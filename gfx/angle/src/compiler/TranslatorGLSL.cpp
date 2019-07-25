





#include "compiler/TranslatorGLSL.h"

#include "compiler/OutputGLSL.h"

TranslatorGLSL::TranslatorGLSL(ShShaderType type, ShShaderSpec spec)
    : TCompiler(type, spec) {
}

void TranslatorGLSL::translate(TIntermNode* root) {
    TOutputGLSL outputGLSL(getInfoSink().obj);
    root->traverse(&outputGLSL);
}
