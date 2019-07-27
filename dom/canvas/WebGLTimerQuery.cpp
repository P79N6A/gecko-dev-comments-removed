





#include "WebGLTimerQuery.h"

#include "GLContext.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"
#include "nsContentUtils.h"
#include "WebGLContext.h"

namespace mozilla {

JSObject*
WebGLTimerQuery::WrapObject(JSContext* cx, JS::Handle<JSObject*> givenProto)
{
  return dom::WebGLTimerQueryEXTBinding::Wrap(cx, this, givenProto);
}

WebGLTimerQuery::WebGLTimerQuery(WebGLContext* webgl, GLuint name)
  : WebGLContextBoundObject(webgl)
  , mGLName(name)
  , mTarget(LOCAL_GL_NONE)
{
}

WebGLTimerQuery::~WebGLTimerQuery()
{
  DeleteOnce();
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

WebGLContext*
WebGLTimerQuery::GetParentObject() const
{
  return Context();
}


NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(WebGLTimerQuery)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(WebGLTimerQuery, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(WebGLTimerQuery, Release)

} 
