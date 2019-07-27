




#include "WebGLExtensions.h"

#include "mozilla/dom/WebGLRenderingContextBinding.h"
#include "WebGLContext.h"

namespace mozilla {

WebGLExtensionDebugRendererInfo::WebGLExtensionDebugRendererInfo(WebGLContext* webgl)
    : WebGLExtensionBase(webgl)
{
}

WebGLExtensionDebugRendererInfo::~WebGLExtensionDebugRendererInfo()
{
}

IMPL_WEBGL_EXTENSION_GOOP(WebGLExtensionDebugRendererInfo)

} 
