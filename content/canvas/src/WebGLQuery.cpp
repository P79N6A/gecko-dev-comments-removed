




#include "WebGLContext.h"
#include "GLContext.h"
#include "WebGLQuery.h"
#include "mozilla/dom/WebGL2RenderingContextBinding.h"
#include "nsContentUtils.h"

using namespace mozilla;

JSObject*
WebGLQuery::WrapObject(JSContext *cx) {
    return dom::WebGLQueryBinding::Wrap(cx, this);
}

WebGLQuery::WebGLQuery(WebGLContext* context)
    : WebGLContextBoundObject(context)
    , mGLName(0)
    , mType(0)
{
    SetIsDOMBinding();
    mContext->mQueries.insertBack(this);

    mContext->MakeContextCurrent();
    mContext->gl->fGenQueries(1, &mGLName);
}

void WebGLQuery::Delete() {
    mContext->MakeContextCurrent();
    mContext->gl->fDeleteQueries(1, &mGLName);
    LinkedListElement<WebGLQuery>::removeFrom(mContext->mQueries);
}

bool WebGLQuery::IsActive() const
{
    WebGLRefPtr<WebGLQuery>* targetSlot = mContext->GetQueryTargetSlot(mType, "WebGLQuery::IsActive()");

    MOZ_ASSERT(targetSlot, "unknown query object's type");

    return targetSlot && *targetSlot == this;
}


NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(WebGLQuery)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(WebGLQuery, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(WebGLQuery, Release)
