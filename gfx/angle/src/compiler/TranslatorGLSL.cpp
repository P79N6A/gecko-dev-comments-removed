





#include "compiler/TranslatorGLSL.h"

#include "compiler/OutputGLSL.h"

TranslatorGLSL::TranslatorGLSL(EShLanguage l, int dOptions)
        : TCompiler(l),
          debugOptions(dOptions) {
}

bool TranslatorGLSL::compile(TIntermNode* root) {
    TOutputGLSL outputGLSL(infoSink.obj);
    root->traverse(&outputGLSL);

    return true;
}
