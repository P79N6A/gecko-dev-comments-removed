




#include "WebGLSync.h"

#include "mozilla/dom/WebGL2RenderingContextBinding.h"

using namespace mozilla;

WebGLSync::WebGLSync(WebGLContext* context) :
    WebGLContextBoundObject(context)
{
    MOZ_CRASH("Not Implemented.");
}

WebGLSync::~WebGLSync()
{}

void
WebGLSync::Delete()
{
    MOZ_CRASH("Not Implemented.");
}

WebGLContext*
WebGLSync::GetParentObject() const
{
    MOZ_CRASH("Not Implemented.");
    return nullptr;
}



JSObject*
WebGLSync::WrapObject(JSContext *cx)
{
    return dom::WebGLSyncBinding::Wrap(cx, this);
}

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(WebGLSync)
NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(WebGLSync, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(WebGLSync, Release);
