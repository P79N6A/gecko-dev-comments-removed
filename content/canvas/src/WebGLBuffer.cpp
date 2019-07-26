




#include "WebGLContext.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"

using namespace mozilla;

JSObject*
WebGLBuffer::WrapObject(JSContext *cx, JSObject *scope, bool *triedToWrap) {
    return dom::WebGLBufferBinding::Wrap(cx, scope, this, triedToWrap);
}

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(WebGLBuffer)

NS_IMPL_CYCLE_COLLECTING_ADDREF(WebGLBuffer)
NS_IMPL_CYCLE_COLLECTING_RELEASE(WebGLBuffer)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(WebGLBuffer)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END
