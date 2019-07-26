




#include "WebGLContext.h"
#include "WebGLExtensions.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"

using namespace mozilla;

WebGLExtensionElementIndexUint::WebGLExtensionElementIndexUint(WebGLContext* context)
    : WebGLExtensionBase(context)
{
}

WebGLExtensionElementIndexUint::~WebGLExtensionElementIndexUint()
{
}

IMPL_WEBGL_EXTENSION_GOOP(WebGLExtensionElementIndexUint)
