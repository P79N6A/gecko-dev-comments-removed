




#include "WebGLContext.h"
#include "WebGLExtensions.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"

using namespace mozilla;

WebGLExtensionDebugRendererInfo::WebGLExtensionDebugRendererInfo(WebGLContext* context)
    : WebGLExtensionBase(context)
{
}

WebGLExtensionDebugRendererInfo::~WebGLExtensionDebugRendererInfo()
{
}

IMPL_WEBGL_EXTENSION_GOOP(WebGLExtensionDebugRendererInfo)
