



#include "WebGLContext.h"
#include "WebGLExtensions.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"

using namespace mozilla;

WebGLExtensionTextureFloatLinear::WebGLExtensionTextureFloatLinear(WebGLContext* context)
    : WebGLExtensionBase(context)
{
}

WebGLExtensionTextureFloatLinear::~WebGLExtensionTextureFloatLinear()
{
}

IMPL_WEBGL_EXTENSION_GOOP(WebGLExtensionTextureFloatLinear)
