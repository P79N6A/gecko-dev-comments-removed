




#include "WebGLActiveInfo.h"

#include "mozilla/dom/WebGLRenderingContextBinding.h"
#include "WebGLContext.h"
#include "WebGLTexture.h"

namespace mozilla {

JSObject*
WebGLActiveInfo::WrapObject(JSContext* cx)
{
    return dom::WebGLActiveInfoBinding::Wrap(cx, this);
}

} 
