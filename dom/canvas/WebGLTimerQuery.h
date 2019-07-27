





#ifndef WEBGL_TIMER_QUERY_H_
#define WEBGL_TIMER_QUERY_H_

#include "nsWrapperCache.h"
#include "WebGLObjectModel.h"

namespace mozilla {

class WebGLTimerQuery final
  : public nsWrapperCache
  , public WebGLBindableName<QueryBinding>
  , public WebGLRefCountedObject<WebGLTimerQuery>
  , public WebGLContextBoundObject
{
public:
  static WebGLTimerQuery* Create(WebGLContext* webgl);

  
  void Delete();

  
  WebGLContext* GetParentObject() const {
    return Context();
  }

  
  virtual JSObject* WrapObject(JSContext* cx, JS::Handle<JSObject*> aGivenProto) override;

  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(WebGLTimerQuery)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(WebGLTimerQuery)

private:
  explicit WebGLTimerQuery(WebGLContext* webgl, GLuint aName);
  ~WebGLTimerQuery() {
    DeleteOnce();
  }

  friend class WebGLExtensionDisjointTimerQuery;
};

} 

#endif 
