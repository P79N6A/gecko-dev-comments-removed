




#include "WebGLVertexArray.h"

#include "WebGLContext.h"
#include "WebGLBuffer.h"
#include "WebGLVertexArrayGL.h"
#include "WebGLVertexArrayFake.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"
#include "GLContext.h"

using namespace mozilla;

JSObject*
WebGLVertexArray::WrapObject(JSContext *cx) {
    return dom::WebGLVertexArrayBinding::Wrap(cx, this);
}

WebGLVertexArray::WebGLVertexArray(WebGLContext* context)
    : WebGLBindableName<GLenum>()
    , WebGLContextBoundObject(context)
{
    SetIsDOMBinding();
    context->mVertexArrays.insertBack(this);
}

WebGLVertexArray*
WebGLVertexArray::Create(WebGLContext* context)
{
    WebGLVertexArray* array;
    if (context->gl->IsSupported(gl::GLFeature::vertex_array_object)) {
        array = new WebGLVertexArrayGL(context);
    } else {
        array = new WebGLVertexArrayFake(context);
    }

    return array;
}

void
WebGLVertexArray::Delete()
{
    DeleteImpl();

    LinkedListElement<WebGLVertexArray>::removeFrom(mContext->mVertexArrays);
    mElementArrayBuffer = nullptr;
    mAttribs.Clear();
}

bool
WebGLVertexArray::EnsureAttrib(GLuint index, const char *info)
{
    if (index >= GLuint(mContext->mGLMaxVertexAttribs)) {
        if (index == GLuint(-1)) {
            mContext->ErrorInvalidValue("%s: index -1 is invalid. That probably comes from a getAttribLocation() call, "
                                        "where this return value -1 means that the passed name didn't correspond to an active attribute in "
                                        "the specified program.", info);
        } else {
            mContext->ErrorInvalidValue("%s: index %d is out of range", info, index);
        }
        return false;
    }
    else if (index >= mAttribs.Length()) {
        mAttribs.SetLength(index + 1);
    }

    return true;
}

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(WebGLVertexArray,
  mAttribs,
  mElementArrayBuffer)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(WebGLVertexArray, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(WebGLVertexArray, Release)
