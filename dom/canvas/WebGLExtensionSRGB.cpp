




#include "WebGLContext.h"
#include "WebGLExtensions.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"
#include "GLContext.h"

using namespace mozilla;

WebGLExtensionSRGB::WebGLExtensionSRGB(WebGLContext* context)
    : WebGLExtensionBase(context)
{
    MOZ_ASSERT(IsSupported(context), "should not construct WebGLExtensionSRGB: "
                                     "sRGB is unsupported.");
    gl::GLContext* gl = context->GL();
    if (!gl->IsGLES()) {
        
        
        gl->MakeCurrent();
        gl->fEnable(LOCAL_GL_FRAMEBUFFER_SRGB_EXT);
    }
}

WebGLExtensionSRGB::~WebGLExtensionSRGB()
{
}

bool
WebGLExtensionSRGB::IsSupported(const WebGLContext* context)
{
    gl::GLContext* gl = context->GL();

    return gl->IsSupported(gl::GLFeature::sRGB);
}


IMPL_WEBGL_EXTENSION_GOOP(WebGLExtensionSRGB)
