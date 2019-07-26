




#include "WebGLContext.h"
#include "WebGLExtensions.h"
#include "nsContentUtils.h"

using namespace mozilla;

WebGLExtensionBase::WebGLExtensionBase(WebGLContext* context)
    : WebGLContextBoundObject(context)
{
    SetIsDOMBinding();
}

WebGLExtensionBase::~WebGLExtensionBase()
{
}

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(WebGLExtensionBase)

NS_IMPL_CYCLE_COLLECTING_ADDREF(WebGLExtensionBase)
NS_IMPL_CYCLE_COLLECTING_RELEASE(WebGLExtensionBase)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(WebGLExtensionBase)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END
