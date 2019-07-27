





#include "compiler/translator/TranslatorESSL.h"

#include "compiler/translator/OutputESSL.h"
#include "angle_gl.h"

TranslatorESSL::TranslatorESSL(sh::GLenum type, ShShaderSpec spec)
    : TCompiler(type, spec, SH_ESSL_OUTPUT) {
}

void TranslatorESSL::translate(TIntermNode* root) {
    TInfoSinkBase& sink = getInfoSink().obj;

    writePragma();

    
    writeExtensionBehavior();

    
    getBuiltInFunctionEmulator().OutputEmulatedFunctionDefinition(
        sink, getShaderType() == GL_FRAGMENT_SHADER);

    
    getArrayBoundsClamper().OutputClampingFunctionDefinition(sink);

    
    TOutputESSL outputESSL(sink, getArrayIndexClampingStrategy(), getHashFunction(), getNameMap(), getSymbolTable(), getShaderVersion());
    root->traverse(&outputESSL);
}

void TranslatorESSL::writeExtensionBehavior() {
    TInfoSinkBase& sink = getInfoSink().obj;
    const TExtensionBehavior& extensionBehavior = getExtensionBehavior();
    for (TExtensionBehavior::const_iterator iter = extensionBehavior.begin();
         iter != extensionBehavior.end(); ++iter) {
        if (iter->second != EBhUndefined) {
            if (getResources().NV_draw_buffers && iter->first == "GL_EXT_draw_buffers") {
                sink << "#extension GL_NV_draw_buffers : "
                     << getBehaviorString(iter->second) << "\n";
            } else {
                sink << "#extension " << iter->first << " : "
                     << getBehaviorString(iter->second) << "\n";
            }
        }
    }
}
