





































#include <stdarg.h>

#include "WebGLContext.h"
#include "WebGLExtensions.h"

#include "nsContentUtils.h"
#include "mozilla/Preferences.h"

using namespace mozilla;

WebGLExtensionTextureFilterAnisotropic::WebGLExtensionTextureFilterAnisotropic(WebGLContext* context) :
    WebGLExtension(context)
{

}

WebGLExtensionTextureFilterAnisotropic::~WebGLExtensionTextureFilterAnisotropic()
{

}

NS_IMPL_ADDREF_INHERITED(WebGLExtensionTextureFilterAnisotropic, WebGLExtension)
NS_IMPL_RELEASE_INHERITED(WebGLExtensionTextureFilterAnisotropic, WebGLExtension)

DOMCI_DATA(WebGLExtensionTextureFilterAnisotropic, WebGLExtensionTextureFilterAnisotropic)

NS_INTERFACE_MAP_BEGIN(WebGLExtensionTextureFilterAnisotropic)
  NS_INTERFACE_MAP_ENTRY(nsIWebGLExtensionTextureFilterAnisotropic)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, WebGLExtension)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(WebGLExtensionTextureFilterAnisotropic)
NS_INTERFACE_MAP_END_INHERITING(WebGLExtension)
