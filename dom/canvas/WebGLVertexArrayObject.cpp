





#include "WebGLVertexArrayObject.h"

#include "mozilla/dom/WebGL2RenderingContextBinding.h"

namespace mozilla {
namespace dom {

WebGLVertexArray*
WebGLVertexArrayObject::Create(WebGLContext* webgl)
{
  
  
  bool vaoSupport = webgl->GL()->IsSupported(gl::GLFeature::vertex_array_object);
  MOZ_RELEASE_ASSERT(vaoSupport, "Vertex Array Objects aren't supported.");
  if (vaoSupport)
    return new WebGLVertexArrayObject(webgl);

  return nullptr;
}

JSObject*
WebGLVertexArrayObject::WrapObject(JSContext* cx,
                                   JS::Handle<JSObject*> givenProto)
{
  return dom::WebGLVertexArrayObjectBinding::Wrap(cx, this, givenProto);
}

} 
} 
