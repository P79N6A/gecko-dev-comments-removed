



#include "WebGLExtensions.h"

#include "mozilla/dom/WebGLRenderingContextBinding.h"
#include "WebGLContext.h"

namespace mozilla {

WebGLExtensionTextureFloatLinear::WebGLExtensionTextureFloatLinear(WebGLContext* webgl)
    : WebGLExtensionBase(webgl)
{
}

WebGLExtensionTextureFloatLinear::~WebGLExtensionTextureFloatLinear()
{
}

IMPL_WEBGL_EXTENSION_GOOP(WebGLExtensionTextureFloatLinear, OES_texture_float_linear)

} 
