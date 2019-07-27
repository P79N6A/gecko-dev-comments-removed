




#include "WebGLExtensions.h"

#include "mozilla/dom/WebGLRenderingContextBinding.h"
#include "WebGLContext.h"

namespace mozilla {

WebGLExtensionShaderTextureLod::WebGLExtensionShaderTextureLod(WebGLContext* webgl)
    : WebGLExtensionBase(webgl)
{
}

WebGLExtensionShaderTextureLod::~WebGLExtensionShaderTextureLod()
{
}

IMPL_WEBGL_EXTENSION_GOOP(WebGLExtensionShaderTextureLod)

} 
