




#include "WebGLContext.h"
#include "WebGLExtensions.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"

using namespace mozilla;

WebGLExtensionStandardDerivatives::WebGLExtensionStandardDerivatives(WebGLContext* context)
    : WebGLExtensionBase(context)
{
}

WebGLExtensionStandardDerivatives::~WebGLExtensionStandardDerivatives()
{
}

IMPL_WEBGL_EXTENSION_GOOP(WebGLExtensionStandardDerivatives)
