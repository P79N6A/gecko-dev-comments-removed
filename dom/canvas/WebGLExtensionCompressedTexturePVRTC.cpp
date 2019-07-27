



#include "WebGLContext.h"
#include "WebGLExtensions.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"

using namespace mozilla;

WebGLExtensionCompressedTexturePVRTC::WebGLExtensionCompressedTexturePVRTC(WebGLContext* context)
    : WebGLExtensionBase(context)
{
    context->mCompressedTextureFormats.AppendElement(LOCAL_GL_COMPRESSED_RGB_PVRTC_4BPPV1);
    context->mCompressedTextureFormats.AppendElement(LOCAL_GL_COMPRESSED_RGB_PVRTC_2BPPV1);
    context->mCompressedTextureFormats.AppendElement(LOCAL_GL_COMPRESSED_RGBA_PVRTC_4BPPV1);
    context->mCompressedTextureFormats.AppendElement(LOCAL_GL_COMPRESSED_RGBA_PVRTC_2BPPV1);
}

WebGLExtensionCompressedTexturePVRTC::~WebGLExtensionCompressedTexturePVRTC()
{
}

IMPL_WEBGL_EXTENSION_GOOP(WebGLExtensionCompressedTexturePVRTC)
