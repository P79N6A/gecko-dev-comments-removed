




#include "WebGLSync.h"

#include "mozilla/dom/WebGL2RenderingContextBinding.h"

namespace mozilla {

WebGLSync::WebGLSync(WebGLContext* webgl, GLenum condition, GLbitfield flags)
    : WebGLContextBoundObject(webgl)
{
   mGLName = mContext->gl->fFenceSync(condition, flags);
}

WebGLSync::~WebGLSync()
{
    DeleteOnce();
}

void
WebGLSync::Delete()
{
    mContext->MakeContextCurrent();
    mContext->gl->fDeleteSync(mGLName);
    mGLName = 0;
    LinkedListElement<WebGLSync>::remove();
}

WebGLContext*
WebGLSync::GetParentObject() const
{
    return Context();
}



JSObject*
WebGLSync::WrapObject(JSContext* cx, JS::Handle<JSObject*> aGivenProto)
{
    return dom::WebGLSyncBinding::Wrap(cx, this, aGivenProto);
}

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(WebGLSync)
NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(WebGLSync, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(WebGLSync, Release);

} 
