




#include "WebGLExtensions.h"

#include "GLContext.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"
#include "WebGLBuffer.h"
#include "WebGLContext.h"
#include "WebGLVertexArray.h"

namespace mozilla {

WebGLExtensionVertexArray::WebGLExtensionVertexArray(WebGLContext* webgl)
  : WebGLExtensionBase(webgl)
{
}

WebGLExtensionVertexArray::~WebGLExtensionVertexArray()
{
}

already_AddRefed<WebGLVertexArray>
WebGLExtensionVertexArray::CreateVertexArrayOES()
{
    if (mIsLost) {
        mContext->ErrorInvalidOperation("%s: Extension is lost.",
                                        "createVertexArrayOES");
        return nullptr;
    }

    return mContext->CreateVertexArray();
}

void
WebGLExtensionVertexArray::DeleteVertexArrayOES(WebGLVertexArray* array)
{
    if (mIsLost) {
        mContext->ErrorInvalidOperation("%s: Extension is lost.",
                                        "deleteVertexArrayOES");
        return;
    }

    mContext->DeleteVertexArray(array);
}

bool
WebGLExtensionVertexArray::IsVertexArrayOES(WebGLVertexArray* array)
{
    if (mIsLost) {
        mContext->ErrorInvalidOperation("%s: Extension is lost.",
                                        "isVertexArrayOES");
        return false;
    }

    return mContext->IsVertexArray(array);
}

void
WebGLExtensionVertexArray::BindVertexArrayOES(WebGLVertexArray* array)
{
    if (mIsLost) {
        mContext->ErrorInvalidOperation("%s: Extension is lost.",
                                        "bindVertexArrayOES");
        return;
    }

    mContext->BindVertexArray(array);
}

IMPL_WEBGL_EXTENSION_GOOP(WebGLExtensionVertexArray)

} 
