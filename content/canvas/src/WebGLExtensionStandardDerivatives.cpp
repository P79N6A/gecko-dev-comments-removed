





































#include <stdarg.h>

#include "WebGLContext.h"
#include "WebGLExtensions.h"

#include "nsContentUtils.h"
#include "mozilla/Preferences.h"

using namespace mozilla;

WebGLExtensionStandardDerivatives::WebGLExtensionStandardDerivatives(WebGLContext* context) :
    WebGLExtension(context)
{

}

WebGLExtensionStandardDerivatives::~WebGLExtensionStandardDerivatives()
{

}

NS_IMPL_ADDREF_INHERITED(WebGLExtensionStandardDerivatives, WebGLExtension)
NS_IMPL_RELEASE_INHERITED(WebGLExtensionStandardDerivatives, WebGLExtension)

DOMCI_DATA(WebGLExtensionStandardDerivatives, WebGLExtensionStandardDerivatives)

NS_INTERFACE_MAP_BEGIN(WebGLExtensionStandardDerivatives)
  NS_INTERFACE_MAP_ENTRY(nsIWebGLExtensionStandardDerivatives)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, WebGLExtension)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(WebGLExtensionStandardDerivatives)
NS_INTERFACE_MAP_END_INHERITING(WebGLExtension)
