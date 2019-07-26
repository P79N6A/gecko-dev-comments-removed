



#include "WebGLContext.h"
#include "WebGLExtensions.h"

using namespace mozilla;

WebGLExtensionCompressedTextureATC::WebGLExtensionCompressedTextureATC(WebGLContext* context)
    : WebGLExtension(context)
{
    context->mCompressedTextureFormats.AppendElement(LOCAL_GL_ATC_RGB);
    context->mCompressedTextureFormats.AppendElement(LOCAL_GL_ATC_RGBA_EXPLICIT_ALPHA);
    context->mCompressedTextureFormats.AppendElement(LOCAL_GL_ATC_RGBA_INTERPOLATED_ALPHA);
}

WebGLExtensionCompressedTextureATC::~WebGLExtensionCompressedTextureATC()
{

}

NS_IMPL_ADDREF_INHERITED(WebGLExtensionCompressedTextureATC, WebGLExtension)
NS_IMPL_RELEASE_INHERITED(WebGLExtensionCompressedTextureATC, WebGLExtension)

DOMCI_DATA(WebGLExtensionCompressedTextureATC, WebGLExtensionCompressedTextureATC)

NS_INTERFACE_MAP_BEGIN(WebGLExtensionCompressedTextureATC)
  NS_INTERFACE_MAP_ENTRY(nsIWebGLExtensionCompressedTextureATC)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, WebGLExtension)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(WebGLExtensionCompressedTextureATC)
NS_INTERFACE_MAP_END_INHERITING(WebGLExtension)
