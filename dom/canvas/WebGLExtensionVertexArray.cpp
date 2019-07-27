




#include "WebGLContext.h"
#include "WebGLBuffer.h"
#include "WebGLVertexArray.h"
#include "WebGLExtensions.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"
#include "GLContext.h"

using namespace mozilla;

WebGLExtensionVertexArray::WebGLExtensionVertexArray(WebGLContext* context)
  : WebGLExtensionBase(context)
{
    MOZ_ASSERT(IsSupported(context), "should not construct WebGLExtensionVertexArray :"
                                     "OES_vertex_array_object unsuported.");
}

WebGLExtensionVertexArray::~WebGLExtensionVertexArray()
{
}

already_AddRefed<WebGLVertexArray> WebGLExtensionVertexArray::CreateVertexArrayOES()
{
    if (mIsLost) {
        mContext->GenerateWarning("createVertexArrayOES: Extension is lost. Returning null.");
        return nullptr;
    }

    return mContext->CreateVertexArray();
}

void WebGLExtensionVertexArray::DeleteVertexArrayOES(WebGLVertexArray* array)
{
    if (mIsLost)
        return mContext->GenerateWarning("deleteVertexArrayOES: Extension is lost.");

    mContext->DeleteVertexArray(array);
}

bool WebGLExtensionVertexArray::IsVertexArrayOES(WebGLVertexArray* array)
{
    if (mIsLost) {
        mContext->GenerateWarning("isVertexArrayOES: Extension is lost. Returning false.");
        return false;
    }

    return mContext->IsVertexArray(array);
}

void WebGLExtensionVertexArray::BindVertexArrayOES(WebGLVertexArray* array)
{
    if (mIsLost)
        return mContext->GenerateWarning("bindVertexArrayOES: Extension is lost.");

    mContext->BindVertexArray(array);
}

bool WebGLExtensionVertexArray::IsSupported(const WebGLContext* context)
{
    
    
    return true;
}

IMPL_WEBGL_EXTENSION_GOOP(WebGLExtensionVertexArray)
