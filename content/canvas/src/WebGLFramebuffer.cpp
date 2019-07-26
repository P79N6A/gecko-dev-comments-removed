




#include "WebGLContext.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"

using namespace mozilla;

JSObject*
WebGLFramebuffer::WrapObject(JSContext *cx, JSObject *scope, bool *triedToWrap) {
    return dom::WebGLFramebufferBinding::Wrap(cx, scope, this, triedToWrap);
}

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(WebGLFramebuffer)

NS_IMPL_CYCLE_COLLECTING_ADDREF(WebGLFramebuffer)
NS_IMPL_CYCLE_COLLECTING_RELEASE(WebGLFramebuffer)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(WebGLFramebuffer)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END
