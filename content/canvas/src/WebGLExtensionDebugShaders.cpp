




#include "WebGLContext.h"
#include "WebGLExtensions.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"

using namespace mozilla;

WebGLExtensionDebugShaders::WebGLExtensionDebugShaders(WebGLContext* context)
    : WebGLExtensionBase(context)
{
}

WebGLExtensionDebugShaders::~WebGLExtensionDebugShaders()
{
}





void
WebGLExtensionDebugShaders::GetTranslatedShaderSource(WebGLShader* shader,
                                                      nsAString& retval)
{
    if (mIsLost) {
        return mContext->ErrorInvalidOperation("getTranslatedShaderSource: "
                                               "Extension is lost.");
    }

    mContext->GetShaderTranslatedSource(shader, retval);

    if (retval.IsVoid()) {
        CopyASCIItoUTF16("", retval);
    }
}

IMPL_WEBGL_EXTENSION_GOOP(WebGLExtensionDebugShaders)
