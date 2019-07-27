




#include "WebGLContext.h"
#include "WebGLExtensions.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"

using namespace mozilla;

WebGLExtensionDepthTexture::WebGLExtensionDepthTexture(WebGLContext* context)
    : WebGLExtensionBase(context)
{
}

WebGLExtensionDepthTexture::~WebGLExtensionDepthTexture()
{
}

IMPL_WEBGL_EXTENSION_GOOP(WebGLExtensionDepthTexture)
