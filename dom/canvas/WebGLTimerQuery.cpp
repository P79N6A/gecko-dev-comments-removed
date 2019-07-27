





#include "WebGLTimerQuery.h"

#include "GLContext.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"
#include "nsContentUtils.h"
#include "WebGLContext.h"

namespace mozilla {

JSObject*
WebGLTimerQuery::WrapObject(JSContext* cx, JS::Handle<JSObject*> aGivenProto)
{
  return dom::WebGLTimerQueryBinding::Wrap(cx, this, aGivenProto);
}

WebGLTimerQuery::WebGLTimerQuery(WebGLContext* webgl, GLuint aName)
  : WebGLBindableName<QueryBinding>(aName)
  , WebGLContextBoundObject(webgl)
{
}

WebGLTimerQuery*
WebGLTimerQuery::Create(WebGLContext* webgl)
{
  GLuint name = 0;
  webgl->MakeContextCurrent();
  webgl->gl->fGenQueries(1, &name);
  return new WebGLTimerQuery(webgl, name);
}

void
WebGLTimerQuery::Delete()
{
  mContext->MakeContextCurrent();
  mContext->gl->fDeleteQueries(1, &mGLName);
}

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(WebGLTimerQuery)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(WebGLTimerQuery, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(WebGLTimerQuery, Release)

} 
