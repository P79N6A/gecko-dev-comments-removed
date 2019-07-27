




#include "WebGLExtensions.h"

#include "mozilla/dom/WebGLRenderingContextBinding.h"
#include "WebGLContext.h"

namespace mozilla {

WebGLExtensionDebugShaders::WebGLExtensionDebugShaders(WebGLContext* webgl)
    : WebGLExtensionBase(webgl)
{
}

WebGLExtensionDebugShaders::~WebGLExtensionDebugShaders()
{
}




void
WebGLExtensionDebugShaders::GetTranslatedShaderSource(WebGLShader* shader,
                                                      nsAString& retval)
{
    retval.SetIsVoid(true);

    if (mIsLost) {
        mContext->ErrorInvalidOperation("%s: Extension is lost.",
                                        "getTranslatedShaderSource");
        return;
    }

    retval.SetIsVoid(false);
    mContext->GetShaderTranslatedSource(shader, retval);
}

IMPL_WEBGL_EXTENSION_GOOP(WebGLExtensionDebugShaders)

} 
