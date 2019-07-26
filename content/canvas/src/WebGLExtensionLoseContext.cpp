




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
    if (!mContext->LoseContext())
        mContext->mWebGLError = LOCAL_GL_INVALID_OPERATION;
}

void 
WebGLExtensionLoseContext::RestoreContext()
{
    if (!mContext->RestoreContext())
        mContext->mWebGLError = LOCAL_GL_INVALID_OPERATION;
}

IMPL_WEBGL_EXTENSION_GOOP(WebGLExtensionLoseContext)
