



#include "WebGLContext.h"
#include "WebGLExtensions.h"

using namespace mozilla;

WebGLExtensionCompressedTexturePVRTC::WebGLExtensionCompressedTexturePVRTC(WebGLContext* context)
    : WebGLExtension(context)
{
    context->mCompressedTextureFormats.AppendElement(LOCAL_GL_COMPRESSED_RGB_PVRTC_4BPPV1);
    context->mCompressedTextureFormats.AppendElement(LOCAL_GL_COMPRESSED_RGB_PVRTC_2BPPV1);
    context->mCompressedTextureFormats.AppendElement(LOCAL_GL_COMPRESSED_RGBA_PVRTC_4BPPV1);
    context->mCompressedTextureFormats.AppendElement(LOCAL_GL_COMPRESSED_RGBA_PVRTC_2BPPV1);
}

WebGLExtensionCompressedTexturePVRTC::~WebGLExtensionCompressedTexturePVRTC()
{

}

NS_IMPL_ADDREF_INHERITED(WebGLExtensionCompressedTexturePVRTC, WebGLExtension)
NS_IMPL_RELEASE_INHERITED(WebGLExtensionCompressedTexturePVRTC, WebGLExtension)

DOMCI_DATA(WebGLExtensionCompressedTexturePVRTC, WebGLExtensionCompressedTexturePVRTC)

NS_INTERFACE_MAP_BEGIN(WebGLExtensionCompressedTexturePVRTC)
  NS_INTERFACE_MAP_ENTRY(nsIWebGLExtensionCompressedTexturePVRTC)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, WebGLExtension)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(WebGLExtensionCompressedTexturePVRTC)
NS_INTERFACE_MAP_END_INHERITING(WebGLExtension)
