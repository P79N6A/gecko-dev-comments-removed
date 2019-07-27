




#include "WebGLExtensions.h"

#include "GLContext.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"
#include "WebGLContext.h"

namespace mozilla {

WebGLExtensionSRGB::WebGLExtensionSRGB(WebGLContext* webgl)
    : WebGLExtensionBase(webgl)
{
    MOZ_ASSERT(IsSupported(webgl), "Don't construct extension if unsupported.");

    gl::GLContext* gl = webgl->GL();
    if (!gl->IsGLES()) {
        
        
        gl->MakeCurrent();
        gl->fEnable(LOCAL_GL_FRAMEBUFFER_SRGB_EXT);
    }
}

WebGLExtensionSRGB::~WebGLExtensionSRGB()
{
}

bool
WebGLExtensionSRGB::IsSupported(const WebGLContext* webgl)
{
    gl::GLContext* gl = webgl->GL();

    return gl->IsSupported(gl::GLFeature::sRGB_framebuffer) &&
           gl->IsSupported(gl::GLFeature::sRGB_texture);
}


IMPL_WEBGL_EXTENSION_GOOP(WebGLExtensionSRGB)

} 
