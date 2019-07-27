




#include "WebGLActiveInfo.h"

#include "mozilla/dom/WebGLRenderingContextBinding.h"
#include "WebGLContext.h"
#include "WebGLTexture.h"

namespace mozilla {

bool
WebGLActiveInfo::WrapObject(JSContext* aCx,
                            JS::MutableHandle<JSObject*> aReflector)
{
    return dom::WebGLActiveInfoBinding::Wrap(aCx, this, aReflector);
}

} 
