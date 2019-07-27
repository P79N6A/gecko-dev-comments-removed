




#include "WebGL1Context.h"

#include "mozilla/dom/WebGLRenderingContextBinding.h"
#include "mozilla/Telemetry.h"

namespace mozilla {

 WebGL1Context*
WebGL1Context::Create()
{
    return new WebGL1Context();
}

WebGL1Context::WebGL1Context()
    : WebGLContext()
{
}

WebGL1Context::~WebGL1Context()
{
}




JSObject*
WebGL1Context::WrapObject(JSContext* cx)
{
    return dom::WebGLRenderingContextBinding::Wrap(cx, this);
}

} 




nsresult
NS_NewCanvasRenderingContextWebGL(nsIDOMWebGLRenderingContext** out_result)
{
    mozilla::Telemetry::Accumulate(mozilla::Telemetry::CANVAS_WEBGL_USED, 1);

    nsIDOMWebGLRenderingContext* ctx = mozilla::WebGL1Context::Create();

    NS_ADDREF(*out_result = ctx);
    return NS_OK;
}
