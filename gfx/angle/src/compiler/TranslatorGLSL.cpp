





#include "compiler/TranslatorGLSL.h"

#include "compiler/OutputGLSL.h"

TranslatorGLSL::TranslatorGLSL(EShLanguage lang, EShSpec spec)
    : TCompiler(lang, spec) {
}

bool TranslatorGLSL::compile(TIntermNode* root) {
    TOutputGLSL outputGLSL(infoSink.obj);
    root->traverse(&outputGLSL);

    return true;
}
