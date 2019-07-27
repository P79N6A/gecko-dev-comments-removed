



#include "WebGLExtensions.h"

#include "mozilla/dom/WebGLRenderingContextBinding.h"
#include "WebGLContext.h"

using namespace mozilla;

WebGLExtensionCompressedTextureETC1::WebGLExtensionCompressedTextureETC1(WebGLContext* context)
    : WebGLExtensionBase(context)
{
    context->mCompressedTextureFormats.AppendElement(LOCAL_GL_ETC1_RGB8_OES);
}

WebGLExtensionCompressedTextureETC1::~WebGLExtensionCompressedTextureETC1()
{
}

IMPL_WEBGL_EXTENSION_GOOP(WebGLExtensionCompressedTextureETC1)
