




#include "WebGLContext.h"
#include "WebGLRenderbuffer.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"
#include "nsContentUtils.h"

using namespace mozilla;

JSObject*
WebGLRenderbuffer::WrapObject(JSContext *cx, JSObject *scope) {
    return dom::WebGLRenderbufferBinding::Wrap(cx, scope, this);
}

WebGLRenderbuffer::WebGLRenderbuffer(WebGLContext *context)
    : WebGLContextBoundObject(context)
    , mInternalFormat(0)
    , mInternalFormatForGL(0)
    , mHasEverBeenBound(false)
    , mInitialized(false)
{
    SetIsDOMBinding();
    mContext->MakeContextCurrent();
    mContext->gl->fGenRenderbuffers(1, &mGLName);
    mContext->mRenderbuffers.insertBack(this);
}

void
WebGLRenderbuffer::Delete() {
    mContext->MakeContextCurrent();
    mContext->gl->fDeleteRenderbuffers(1, &mGLName);
    LinkedListElement<WebGLRenderbuffer>::removeFrom(mContext->mRenderbuffers);
}

int64_t
WebGLRenderbuffer::MemoryUsage() const {
    int64_t pixels = int64_t(Width()) * int64_t(Height());

    
    if (!mInternalFormatForGL) {
        return 0;
    }

    switch (mInternalFormatForGL) {
        case LOCAL_GL_STENCIL_INDEX8:
            return pixels;
        case LOCAL_GL_RGBA4:
        case LOCAL_GL_RGB5_A1:
        case LOCAL_GL_RGB565:
        case LOCAL_GL_DEPTH_COMPONENT16:
            return 2 * pixels;
        case LOCAL_GL_RGB8:
        case LOCAL_GL_DEPTH_COMPONENT24:
            return 3*pixels;
        case LOCAL_GL_RGBA8:
        case LOCAL_GL_DEPTH24_STENCIL8:
            return 4*pixels;
        default:
            break;
    }
    NS_ABORT();
    return 0;
}

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(WebGLRenderbuffer)

NS_IMPL_CYCLE_COLLECTING_ADDREF(WebGLRenderbuffer)
NS_IMPL_CYCLE_COLLECTING_RELEASE(WebGLRenderbuffer)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(WebGLRenderbuffer)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END
