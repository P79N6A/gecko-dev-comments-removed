




#include "WebGLContext.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"

using namespace mozilla;

JSObject*
WebGLActiveInfo::WrapObject(JSContext *cx, JSObject *scope) {
    return dom::WebGLActiveInfoBinding::Wrap(cx, scope, this);
}

NS_IMPL_ADDREF(WebGLActiveInfo)
NS_IMPL_RELEASE(WebGLActiveInfo)

NS_INTERFACE_MAP_BEGIN(WebGLActiveInfo)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END
