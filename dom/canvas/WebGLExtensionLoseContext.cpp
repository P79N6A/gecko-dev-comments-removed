




#include "WebGLExtensions.h"

#include "mozilla/dom/WebGLRenderingContextBinding.h"
#include "WebGLContext.h"

namespace mozilla {

WebGLExtensionLoseContext::WebGLExtensionLoseContext(WebGLContext* webgl)
    : WebGLExtensionBase(webgl)
{
}

WebGLExtensionLoseContext::~WebGLExtensionLoseContext()
{
}

void
WebGLExtensionLoseContext::LoseContext()
{
    mContext->LoseContext();
}

void
WebGLExtensionLoseContext::RestoreContext()
{
    mContext->RestoreContext();
}

IMPL_WEBGL_EXTENSION_GOOP(WebGLExtensionLoseContext)

} 
