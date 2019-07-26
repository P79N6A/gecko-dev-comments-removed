




#include "WebGLContext.h"
#include "WebGLExtensions.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"

using namespace mozilla;

WebGLExtensionLoseContext::WebGLExtensionLoseContext(WebGLContext* context)
    : WebGLExtensionBase(context)
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
