




#include "WebGLVertexArray.h"

#include "GLContext.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"
#include "WebGLBuffer.h"
#include "WebGLContext.h"
#include "WebGLVertexArrayGL.h"
#include "WebGLVertexArrayFake.h"

namespace mozilla {

JSObject*
WebGLVertexArray::WrapObject(JSContext* cx)
{
    return dom::WebGLVertexArrayBinding::Wrap(cx, this);
}

WebGLVertexArray::WebGLVertexArray(WebGLContext* webgl)
    : WebGLBindable<VAOBinding>()
    , WebGLContextBoundObject(webgl)
    , mGLName(0)
{
    mContext->mVertexArrays.insertBack(this);
}

WebGLVertexArray*
WebGLVertexArray::Create(WebGLContext* webgl)
{
    WebGLVertexArray* array;
    if (webgl->gl->IsSupported(gl::GLFeature::vertex_array_object)) {
        array = new WebGLVertexArrayGL(webgl);
    } else {
        array = new WebGLVertexArrayFake(webgl);
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

} 
