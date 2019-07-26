




#include "WebGLContext.h"
#include "WebGLTexture.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"

using namespace mozilla;

JSObject*
WebGLActiveInfo::WrapObject(JSContext *cx, JS::Handle<JSObject*> scope) {
    return dom::WebGLActiveInfoBinding::Wrap(cx, scope, this);
}
