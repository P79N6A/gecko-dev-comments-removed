




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
    : WebGLBindable<VAOBinding>()
    , WebGLContextBoundObject(context)
    , mGLName(0)
{
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

void
WebGLVertexArray::EnsureAttrib(GLuint index)
{
    MOZ_ASSERT(index < GLuint(mContext->mGLMaxVertexAttribs));

    if (index >= mAttribs.Length()) {
        mAttribs.SetLength(index + 1);
    }
}

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(WebGLVertexArray,
  mAttribs,
  mElementArrayBuffer)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(WebGLVertexArray, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(WebGLVertexArray, Release)
