




#include "WebGLExtensions.h"

#include "mozilla/dom/WebGLRenderingContextBinding.h"
#include "WebGLContext.h"

namespace mozilla {

WebGLExtensionDepthTexture::WebGLExtensionDepthTexture(WebGLContext* webgl)
    : WebGLExtensionBase(webgl)
{
}

WebGLExtensionDepthTexture::~WebGLExtensionDepthTexture()
{
}

IMPL_WEBGL_EXTENSION_GOOP(WebGLExtensionDepthTexture)

} 
