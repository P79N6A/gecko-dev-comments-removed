





































#include <stdarg.h>

#include "WebGLContext.h"
#include "WebGLExtensions.h"

#include "nsContentUtils.h"
#include "mozilla/Preferences.h"

using namespace mozilla;

WebGLExtensionLoseContext::WebGLExtensionLoseContext(WebGLContext* context) :
    WebGLExtension(context)
{

}

WebGLExtensionLoseContext::~WebGLExtensionLoseContext()
{

}

NS_IMETHODIMP 
WebGLExtensionLoseContext::LoseContext()
{
    if (!mContext->LoseContext())
        mContext->mWebGLError = LOCAL_GL_INVALID_OPERATION;

    return NS_OK;
}

NS_IMETHODIMP 
WebGLExtensionLoseContext::RestoreContext()
{
    if (!mContext->RestoreContext())
        mContext->mWebGLError = LOCAL_GL_INVALID_OPERATION;

    return NS_OK;
}
