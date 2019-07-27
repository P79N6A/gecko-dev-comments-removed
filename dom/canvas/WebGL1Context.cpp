




#include "WebGL1Context.h"

#include "mozilla/dom/WebGLRenderingContextBinding.h"
#include "mozilla/Telemetry.h"
#include "WebGLFormats.h"

namespace mozilla {

 WebGL1Context*
WebGL1Context::Create()
{
    return new WebGL1Context();
}

WebGL1Context::WebGL1Context()
    : WebGLContext()
{
    mFormatUsage = Move(webgl::FormatUsageAuthority::CreateForWebGL1());
}

WebGL1Context::~WebGL1Context()
{
}

JSObject*
WebGL1Context::WrapObject(JSContext* cx, JS::Handle<JSObject*> aGivenProto)
{
    return dom::WebGLRenderingContextBinding::Wrap(cx, this, aGivenProto);
}

bool
WebGL1Context::ValidateQueryTarget(GLenum target, const char* info)
{
    
    return false;
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
