



#include "WebGLContext.h"
#include "WebGLExtensions.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"

using namespace mozilla;

WebGLExtensionTextureFloat::WebGLExtensionTextureFloat(WebGLContext* context)
    : WebGLExtensionBase(context)
{
}

WebGLExtensionTextureFloat::~WebGLExtensionTextureFloat()
{
}

IMPL_WEBGL_EXTENSION_GOOP(WebGLExtensionTextureFloat)
