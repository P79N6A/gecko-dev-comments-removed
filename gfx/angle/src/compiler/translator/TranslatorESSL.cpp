





#include "compiler/translator/TranslatorESSL.h"

#include "compiler/translator/OutputESSL.h"

TranslatorESSL::TranslatorESSL(ShShaderType type, ShShaderSpec spec)
    : TCompiler(type, spec, SH_ESSL_OUTPUT) {
}

void TranslatorESSL::translate(TIntermNode* root) {
    TInfoSinkBase& sink = getInfoSink().obj;

    
    writeExtensionBehavior();

    
    getBuiltInFunctionEmulator().OutputEmulatedFunctionDefinition(
        sink, getShaderType() == SH_FRAGMENT_SHADER);

    
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
            sink << "#extension " << iter->first << " : "
                 << getBehaviorString(iter->second) << "\n";
        }
    }
}
