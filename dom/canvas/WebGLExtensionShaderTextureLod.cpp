




#include "WebGLContext.h"
#include "WebGLExtensions.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"

using namespace mozilla;

WebGLExtensionShaderTextureLod::WebGLExtensionShaderTextureLod(WebGLContext* context)
    : WebGLExtensionBase(context)
{
}

WebGLExtensionShaderTextureLod::~WebGLExtensionShaderTextureLod()
{
}

IMPL_WEBGL_EXTENSION_GOOP(WebGLExtensionShaderTextureLod)
