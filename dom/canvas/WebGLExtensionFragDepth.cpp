




#include "WebGLContext.h"
#include "WebGLExtensions.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"
#include "GLContext.h"

using namespace mozilla;

WebGLExtensionFragDepth::WebGLExtensionFragDepth(WebGLContext* context)
    : WebGLExtensionBase(context)
{
    MOZ_ASSERT(IsSupported(context),
               "Should not construct extension object if unsupported.");
}

WebGLExtensionFragDepth::~WebGLExtensionFragDepth()
{
}

bool
WebGLExtensionFragDepth::IsSupported(const WebGLContext* context)
{
    gl::GLContext* gl = context->GL();

    return gl->IsSupported(gl::GLFeature::frag_depth);
}


IMPL_WEBGL_EXTENSION_GOOP(WebGLExtensionFragDepth)
