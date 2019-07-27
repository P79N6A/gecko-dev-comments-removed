




#include "WebGLContext.h"
#include "WebGLExtensions.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"

using namespace mozilla;

WebGLExtensionTextureFilterAnisotropic::WebGLExtensionTextureFilterAnisotropic(WebGLContext* context)
    : WebGLExtensionBase(context)
{
}

WebGLExtensionTextureFilterAnisotropic::~WebGLExtensionTextureFilterAnisotropic()
{
}

IMPL_WEBGL_EXTENSION_GOOP(WebGLExtensionTextureFilterAnisotropic)
