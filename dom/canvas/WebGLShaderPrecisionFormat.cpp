




#include "WebGLShaderPrecisionFormat.h"

#include "mozilla/dom/WebGLRenderingContextBinding.h"
#include "WebGLContext.h"

namespace mozilla {

bool
WebGLShaderPrecisionFormat::WrapObject(JSContext* aCx,
                                       JS::Handle<JSObject*> aGivenProto,
                                       JS::MutableHandle<JSObject*> aReflector)
{
    return dom::WebGLShaderPrecisionFormatBinding::Wrap(aCx, this, aGivenProto, aReflector);
}

} 
