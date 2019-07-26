



#include "WebGLContext.h"
#include "WebGLExtensions.h"

#include "GLContext.h"

#include "mozilla/dom/WebGLRenderingContextBinding.h"

using namespace mozilla;

WebGLExtensionBlendMinMax::WebGLExtensionBlendMinMax(WebGLContext* context)
    : WebGLExtensionBase(context)
{
}

WebGLExtensionBlendMinMax::~WebGLExtensionBlendMinMax()
{
}

bool WebGLExtensionBlendMinMax::IsSupported(const WebGLContext* context)
{
    gl::GLContext * gl = context->GL();

    return gl->IsSupported(GLFeature::blend_minmax);
}

IMPL_WEBGL_EXTENSION_GOOP(WebGLExtensionBlendMinMax)
