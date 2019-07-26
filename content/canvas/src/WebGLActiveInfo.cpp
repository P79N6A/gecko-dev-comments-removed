




#include "WebGLContext.h"
#include "WebGLTexture.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"

using namespace mozilla;

JSObject*
WebGLActiveInfo::WrapObject(JSContext *cx) {
    return dom::WebGLActiveInfoBinding::Wrap(cx, this);
}
