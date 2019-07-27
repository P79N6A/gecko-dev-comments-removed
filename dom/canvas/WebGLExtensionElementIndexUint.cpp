




#include "WebGLExtensions.h"

#include "mozilla/dom/WebGLRenderingContextBinding.h"
#include "WebGLContext.h"

namespace mozilla {

WebGLExtensionElementIndexUint::WebGLExtensionElementIndexUint(WebGLContext* webgl)
    : WebGLExtensionBase(webgl)
{
}

WebGLExtensionElementIndexUint::~WebGLExtensionElementIndexUint()
{
}

IMPL_WEBGL_EXTENSION_GOOP(WebGLExtensionElementIndexUint)

} 
