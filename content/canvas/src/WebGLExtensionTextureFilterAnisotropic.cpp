





































#include <stdarg.h>

#include "WebGLContext.h"
#include "WebGLExtensions.h"

#include "nsContentUtils.h"
#include "mozilla/Preferences.h"

using namespace mozilla;

NS_INTERFACE_MAP_BEGIN(WebGLExtensionTextureFilterAnisotropic)
  NS_INTERFACE_MAP_ENTRY(nsIWebGLExtensionTextureFilterAnisotropic)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, WebGLExtension)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(WebGLExtensionTextureFilterAnisotropic)
NS_INTERFACE_MAP_END_INHERITING(WebGLExtension)

WebGLExtensionTextureFilterAnisotropic::WebGLExtensionTextureFilterAnisotropic(WebGLContext* context) :
    WebGLExtension(context)
{

}

WebGLExtensionTextureFilterAnisotropic::~WebGLExtensionTextureFilterAnisotropic()
{

}
