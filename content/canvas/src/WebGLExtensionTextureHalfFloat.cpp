



#include "WebGLContext.h"
#include "WebGLExtensions.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"

using namespace mozilla;

WebGLExtensionTextureHalfFloat::WebGLExtensionTextureHalfFloat(WebGLContext* context)
    : WebGLExtensionBase(context)
{
}

WebGLExtensionTextureHalfFloat::~WebGLExtensionTextureHalfFloat()
{
}

IMPL_WEBGL_EXTENSION_GOOP(WebGLExtensionTextureHalfFloat)
