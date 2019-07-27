



#include "WebGLExtensions.h"

#include "GLContext.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"
#include "WebGLContext.h"

using namespace mozilla;

WebGLExtensionColorBufferFloat::WebGLExtensionColorBufferFloat(WebGLContext* context)
    : WebGLExtensionBase(context)
{
    MOZ_ASSERT(IsSupported(context));
}

WebGLExtensionColorBufferFloat::~WebGLExtensionColorBufferFloat()
{
}

bool
WebGLExtensionColorBufferFloat::IsSupported(const WebGLContext* context)
{
    return context->GL()->IsSupported(gl::GLFeature::renderbuffer_color_float) &&
           context->GL()->IsSupported(gl::GLFeature::frag_color_float);
}

IMPL_WEBGL_EXTENSION_GOOP(WebGLExtensionColorBufferFloat)
