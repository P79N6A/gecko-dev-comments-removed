




#include "WebGL2Context.h"
#include "mozilla/dom/WebGL2RenderingContextBinding.h"

#include "mozilla/Telemetry.h"

using namespace mozilla;




WebGL2Context::WebGL2Context()
    : WebGLContext()
{
    MOZ_ASSERT(IsSupported(), "not supposed to create a WebGL2Context"
                              "context when not supported");
}

WebGL2Context::~WebGL2Context()
{

}





bool
WebGL2Context::IsSupported()
{
#ifdef RELEASE_BUILD
    return false;
#else
    return Preferences::GetBool("webgl.enable-prototype-webgl2", false);
#endif
}

WebGL2Context*
WebGL2Context::Create()
{
#ifdef RELEASE_BUILD
    return nullptr;
#else
    return new WebGL2Context();
#endif
}





JSObject*
WebGL2Context::WrapObject(JSContext *cx, JS::Handle<JSObject*> scope)
{
    return dom::WebGL2RenderingContextBinding::Wrap(cx, scope, this);
}

