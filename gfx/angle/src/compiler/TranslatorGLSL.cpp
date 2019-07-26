





#include "compiler/TranslatorGLSL.h"

#include "compiler/OutputGLSL.h"
#include "compiler/VersionGLSL.h"

static void writeVersion(ShShaderType type, TIntermNode* root,
                         TInfoSinkBase& sink) {
    TVersionGLSL versionGLSL(type);
    root->traverse(&versionGLSL);
    int version = versionGLSL.getVersion();
    
    
    if (version > 110) {
        sink << "#version " << version << "\n";
    }
}

TranslatorGLSL::TranslatorGLSL(ShShaderType type, ShShaderSpec spec)
    : TCompiler(type, spec) {
}

void TranslatorGLSL::translate(TIntermNode* root) {
    TInfoSinkBase& sink = getInfoSink().obj;

    
    writeVersion(getShaderType(), root, sink);

    
    getBuiltInFunctionEmulator().OutputEmulatedFunctionDefinition(
        sink, false);

    
    getArrayBoundsClamper().OutputClampingFunctionDefinition(sink);

    
    TOutputGLSL outputGLSL(sink, getHashFunction(), getNameMap(), getSymbolTable());
    root->traverse(&outputGLSL);
}
