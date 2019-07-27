




#include "WebGL2Context.h"
#include "GLContext.h"
#include "WebGLVertexArrayObject.h"

namespace mozilla {




WebGLVertexArray*
WebGL2Context::CreateVertexArrayImpl()
{
    return dom::WebGLVertexArrayObject::Create(this);
}

} 
