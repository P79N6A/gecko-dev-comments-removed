




#include "WebGLContext.h"
#include "WebGLQuery.h"
#include "mozilla/dom/WebGL2RenderingContextBinding.h"
#include "nsContentUtils.h"

using namespace mozilla;

JSObject*
WebGLQuery::WrapObject(JSContext *cx, JS::Handle<JSObject*> scope) {
    return dom::WebGLQueryBinding::Wrap(cx, scope, this);
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


NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(WebGLQuery)

NS_IMPL_CYCLE_COLLECTING_ADDREF(WebGLQuery)
NS_IMPL_CYCLE_COLLECTING_RELEASE(WebGLQuery)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(WebGLQuery)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END
