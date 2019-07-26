




#include "WebGLContext.h"
#include "WebGLBuffer.h"
#include "WebGLVertexArray.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"
#include "nsContentUtils.h"

using namespace mozilla;

JSObject*
WebGLVertexArray::WrapObject(JSContext *cx, JS::Handle<JSObject*> scope) {
    return dom::WebGLVertexArrayBinding::Wrap(cx, scope, this);
}

WebGLVertexArray::WebGLVertexArray(WebGLContext* context)
    : WebGLContextBoundObject(context)
    , mGLName(0)
    , mHasEverBeenBound(false)
{
    SetIsDOMBinding();
}

void WebGLVertexArray::Delete() {
    if (mGLName != 0) {
        mBoundElementArrayBuffer = nullptr;

        mContext->MakeContextCurrent();
        mContext->gl->fDeleteVertexArrays(1, &mGLName);
        LinkedListElement<WebGLVertexArray>::removeFrom(mContext->mVertexArrays);
    }

    mBoundElementArrayBuffer = nullptr;
    mAttribBuffers.Clear();
}

bool WebGLVertexArray::EnsureAttribIndex(WebGLuint index, const char *info)
{
    if (index >= WebGLuint(mContext->mGLMaxVertexAttribs)) {
        if (index == WebGLuint(-1)) {
            mContext->ErrorInvalidValue("%s: index -1 is invalid. That probably comes from a getAttribLocation() call, "
                                        "where this return value -1 means that the passed name didn't correspond to an active attribute in "
                                        "the specified program.", info);
        } else {
            mContext->ErrorInvalidValue("%s: index %d is out of range", info, index);
        }
        return false;
    }
    else if (index >= mAttribBuffers.Length()) {
        mAttribBuffers.SetLength(index + 1);
    }

    return true;
}

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_2(WebGLVertexArray,
  mAttribBuffers,
  mBoundElementArrayBuffer)

NS_IMPL_CYCLE_COLLECTING_ADDREF(WebGLVertexArray)
NS_IMPL_CYCLE_COLLECTING_RELEASE(WebGLVertexArray)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(WebGLVertexArray)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END
