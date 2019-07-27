




#include "WebGLExtensions.h"

#include "mozilla/dom/WebGLRenderingContextBinding.h"
#include "WebGLContext.h"

namespace mozilla {

WebGLExtensionStandardDerivatives::WebGLExtensionStandardDerivatives(WebGLContext* webgl)
    : WebGLExtensionBase(webgl)
{
}

WebGLExtensionStandardDerivatives::~WebGLExtensionStandardDerivatives()
{
}

IMPL_WEBGL_EXTENSION_GOOP(WebGLExtensionStandardDerivatives)

} 
