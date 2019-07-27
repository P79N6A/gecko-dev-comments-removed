




#include "WebGLExtensions.h"

#include "GLContext.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"
#include "WebGLContext.h"

namespace mozilla {

WebGLExtensionFragDepth::WebGLExtensionFragDepth(WebGLContext* webgl)
    : WebGLExtensionBase(webgl)
{
    MOZ_ASSERT(IsSupported(webgl), "Don't construct extension if unsupported.");
}

WebGLExtensionFragDepth::~WebGLExtensionFragDepth()
{
}

bool
WebGLExtensionFragDepth::IsSupported(const WebGLContext* webgl)
{
    gl::GLContext* gl = webgl->GL();
    return gl->IsSupported(gl::GLFeature::frag_depth);
}


IMPL_WEBGL_EXTENSION_GOOP(WebGLExtensionFragDepth)

} 
