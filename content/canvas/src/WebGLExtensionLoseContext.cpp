





































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

NS_IMPL_ADDREF_INHERITED(WebGLExtensionLoseContext, WebGLExtension)
NS_IMPL_RELEASE_INHERITED(WebGLExtensionLoseContext, WebGLExtension)

DOMCI_DATA(WebGLExtensionLoseContext, WebGLExtensionLoseContext)

NS_INTERFACE_MAP_BEGIN(WebGLExtensionLoseContext)
  NS_INTERFACE_MAP_ENTRY(nsIWebGLExtensionLoseContext)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, WebGLExtension)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(WebGLExtensionLoseContext)
NS_INTERFACE_MAP_END_INHERITING(WebGLExtension)
