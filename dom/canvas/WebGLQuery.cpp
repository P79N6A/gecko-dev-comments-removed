




#include "WebGLQuery.h"

#include "GLContext.h"
#include "mozilla/dom/WebGL2RenderingContextBinding.h"
#include "nsContentUtils.h"
#include "WebGLContext.h"

namespace mozilla {

JSObject*
WebGLQuery::WrapObject(JSContext* cx, JS::Handle<JSObject*> aGivenProto)
{
    return dom::WebGLQueryBinding::Wrap(cx, this, aGivenProto);
}

WebGLQuery::WebGLQuery(WebGLContext* webgl)
    : WebGLContextBoundObject(webgl)
    , mGLName(0)
    , mType(0)
{
    mContext->mQueries.insertBack(this);

    mContext->MakeContextCurrent();
    mContext->gl->fGenQueries(1, &mGLName);
}

void
WebGLQuery::Delete()
{
    mContext->MakeContextCurrent();
    mContext->gl->fDeleteQueries(1, &mGLName);
    LinkedListElement<WebGLQuery>::removeFrom(mContext->mQueries);
}

bool
WebGLQuery::IsActive() const
{
    WebGLRefPtr<WebGLQuery>& targetSlot = mContext->GetQuerySlotByTarget(mType);

    return targetSlot.get() == this;
}

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(WebGLQuery)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(WebGLQuery, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(WebGLQuery, Release)

} 
