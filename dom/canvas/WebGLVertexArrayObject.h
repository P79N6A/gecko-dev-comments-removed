




#ifndef mozilla_dom_WebGLVertexArrayObject_h
#define mozilla_dom_WebGLVertexArrayObject_h

#include "WebGLVertexArrayGL.h"

namespace mozilla {
namespace dom {








class WebGLVertexArrayObject final
  : public WebGLVertexArrayGL
{
public:
  static WebGLVertexArray* Create(WebGLContext* webgl);

  virtual JSObject* WrapObject(JSContext* cx, JS::Handle<JSObject*> givenProto) override;

private:
  explicit WebGLVertexArrayObject(WebGLContext* webgl)
    : WebGLVertexArrayGL(webgl)
  { }

  ~WebGLVertexArrayObject() {
    DeleteOnce();
  }
};

} 
} 

#endif 
