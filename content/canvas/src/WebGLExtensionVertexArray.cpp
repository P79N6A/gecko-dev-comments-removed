




#include "WebGLContext.h"
#include "WebGLBuffer.h"
#include "WebGLVertexArray.h"
#include "WebGLExtensions.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"

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
    return mContext->CreateVertexArray();
}

void WebGLExtensionVertexArray::DeleteVertexArrayOES(WebGLVertexArray* array)
{
    mContext->DeleteVertexArray(array);
}

bool WebGLExtensionVertexArray::IsVertexArrayOES(WebGLVertexArray* array)
{
    return mContext->IsVertexArray(array);
}

void WebGLExtensionVertexArray::BindVertexArrayOES(WebGLVertexArray* array)
{
    mContext->BindVertexArray(array);
}

bool WebGLExtensionVertexArray::IsSupported(const WebGLContext* context)
{
    



    return false;










}

IMPL_WEBGL_EXTENSION_GOOP(WebGLExtensionVertexArray)
