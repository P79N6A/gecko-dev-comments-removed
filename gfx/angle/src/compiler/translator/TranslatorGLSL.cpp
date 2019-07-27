





#include "compiler/translator/TranslatorGLSL.h"

#include "compiler/translator/OutputGLSL.h"
#include "compiler/translator/VersionGLSL.h"

static void writeVersion(sh::GLenum type, TIntermNode* root,
                         TInfoSinkBase& sink) {
    TVersionGLSL versionGLSL(type);
    root->traverse(&versionGLSL);
    int version = versionGLSL.getVersion();
    
    
    if (version > 110) {
        sink << "#version " << version << "\n";
    }
}

TranslatorGLSL::TranslatorGLSL(sh::GLenum type, ShShaderSpec spec)
    : TCompiler(type, spec, SH_GLSL_OUTPUT) {
}

void TranslatorGLSL::translate(TIntermNode* root) {
    TInfoSinkBase& sink = getInfoSink().obj;

    
    writeVersion(getShaderType(), root, sink);

    
    writeExtensionBehavior();

    
    getBuiltInFunctionEmulator().OutputEmulatedFunctionDefinition(
        sink, false);

    
    getArrayBoundsClamper().OutputClampingFunctionDefinition(sink);

    
    TOutputGLSL outputGLSL(sink, getArrayIndexClampingStrategy(), getHashFunction(), getNameMap(), getSymbolTable(), getShaderVersion());
    root->traverse(&outputGLSL);
}

void TranslatorGLSL::writeExtensionBehavior() {
    TInfoSinkBase& sink = getInfoSink().obj;
    const TExtensionBehavior& extensionBehavior = getExtensionBehavior();
    for (TExtensionBehavior::const_iterator iter = extensionBehavior.begin();
         iter != extensionBehavior.end(); ++iter) {
        if (iter->second == EBhUndefined)
            continue;

        
        
        if (iter->first == "GL_EXT_shader_texture_lod") {
            sink << "#extension GL_ARB_shader_texture_lod : "
                 << getBehaviorString(iter->second) << "\n";
        }
    }
}
