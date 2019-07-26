



#include "WebGLContext.h"
#include "WebGLExtensions.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"

using namespace mozilla;

WebGLExtensionTextureHalfFloatLinear::WebGLExtensionTextureHalfFloatLinear(WebGLContext* context)
    : WebGLExtensionBase(context)
{
}

WebGLExtensionTextureHalfFloatLinear::~WebGLExtensionTextureHalfFloatLinear()
{
}

IMPL_WEBGL_EXTENSION_GOOP(WebGLExtensionTextureHalfFloatLinear)
