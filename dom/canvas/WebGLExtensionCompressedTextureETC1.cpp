



#include "WebGLExtensions.h"

#include "mozilla/dom/WebGLRenderingContextBinding.h"
#include "WebGLContext.h"

namespace mozilla {

WebGLExtensionCompressedTextureETC1::WebGLExtensionCompressedTextureETC1(WebGLContext* webgl)
    : WebGLExtensionBase(webgl)
{
    webgl->mCompressedTextureFormats.AppendElement(LOCAL_GL_ETC1_RGB8_OES);
}

WebGLExtensionCompressedTextureETC1::~WebGLExtensionCompressedTextureETC1()
{
}

IMPL_WEBGL_EXTENSION_GOOP(WebGLExtensionCompressedTextureETC1)

} 
