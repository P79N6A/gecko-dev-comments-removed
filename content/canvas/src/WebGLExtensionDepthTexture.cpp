




#include "WebGLContext.h"
#include "WebGLExtensions.h"

using namespace mozilla;

WebGLExtensionDepthTexture::WebGLExtensionDepthTexture(WebGLContext* context) :
    WebGLExtension(context)
{

}

WebGLExtensionDepthTexture::~WebGLExtensionDepthTexture()
{

}

NS_IMPL_ADDREF_INHERITED(WebGLExtensionDepthTexture, WebGLExtension)
NS_IMPL_RELEASE_INHERITED(WebGLExtensionDepthTexture, WebGLExtension)

DOMCI_DATA(WebGLExtensionDepthTexture, WebGLExtensionDepthTexture)

NS_INTERFACE_MAP_BEGIN(WebGLExtensionDepthTexture)
  NS_INTERFACE_MAP_ENTRY(nsIWebGLExtensionDepthTexture)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, WebGLExtension)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(WebGLExtensionDepthTexture)
NS_INTERFACE_MAP_END_INHERITING(WebGLExtension)

