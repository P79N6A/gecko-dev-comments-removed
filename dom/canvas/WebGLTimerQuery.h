





#ifndef WEBGL_TIMER_QUERY_H_
#define WEBGL_TIMER_QUERY_H_

#include "GLConsts.h"
#include "nsWrapperCache.h"
#include "WebGLObjectModel.h"

namespace mozilla {

class WebGLTimerQuery final
  : public nsWrapperCache
  , public WebGLRefCountedObject<WebGLTimerQuery>
  , public WebGLContextBoundObject
{
public:
  static WebGLTimerQuery* Create(WebGLContext* webgl);

  void Delete();

  bool HasEverBeenBound() const { return mTarget != LOCAL_GL_NONE; }
  GLenum Target() const { return mTarget; }

  WebGLContext* GetParentObject() const;

  
  virtual JSObject* WrapObject(JSContext* cx, JS::Handle<JSObject*> aGivenProto) override;

  const GLenum mGLName;

  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(WebGLTimerQuery)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(WebGLTimerQuery)

private:
  explicit WebGLTimerQuery(WebGLContext* webgl, GLuint aName);
  ~WebGLTimerQuery();

  GLenum mTarget;

  friend class WebGLExtensionDisjointTimerQuery;
};

} 

#endif 
