




#include "WebGLTransformFeedback.h"

#include "GLContext.h"
#include "mozilla/dom/WebGL2RenderingContextBinding.h"
#include "WebGL2Context.h"

namespace mozilla {

WebGLTransformFeedback::WebGLTransformFeedback(WebGLContext* webgl,
                                               GLuint tf)
    : WebGLContextBoundObject(webgl)
    , mGLName(tf)
    , mMode(LOCAL_GL_NONE)
    , mIsActive(false)
    , mIsPaused(false)
{
    mContext->mTransformFeedbacks.insertBack(this);
}

WebGLTransformFeedback::~WebGLTransformFeedback()
{
    mMode = LOCAL_GL_NONE;
    mIsActive = false;
    mIsPaused = false;
    DeleteOnce();
}

void
WebGLTransformFeedback::Delete()
{
    mContext->MakeContextCurrent();
    mContext->gl->fDeleteTransformFeedbacks(1, &mGLName);
    removeFrom(mContext->mTransformFeedbacks);
}

WebGLContext*
WebGLTransformFeedback::GetParentObject() const
{
    return Context();
}

JSObject*
WebGLTransformFeedback::WrapObject(JSContext* cx, JS::Handle<JSObject*> aGivenProto)
{
    return dom::WebGLTransformFeedbackBinding::Wrap(cx, this, aGivenProto);
}


NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(WebGLTransformFeedback)
NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(WebGLTransformFeedback, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(WebGLTransformFeedback, Release)

} 
