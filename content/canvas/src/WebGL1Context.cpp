




#include "WebGL1Context.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"

#include "mozilla/Telemetry.h"

using namespace mozilla;




WebGL1Context::WebGL1Context()
    : WebGLContext()
{

}

WebGL1Context::~WebGL1Context()
{

}





JSObject*
WebGL1Context::WrapObject(JSContext *cx)
{
    return dom::WebGLRenderingContextBinding::Wrap(cx, this);
}





nsresult
NS_NewCanvasRenderingContextWebGL(nsIDOMWebGLRenderingContext** aResult)
{
    Telemetry::Accumulate(Telemetry::CANVAS_WEBGL_USED, 1);
    nsIDOMWebGLRenderingContext* ctx = new WebGL1Context();

    NS_ADDREF(*aResult = ctx);
    return NS_OK;
}

