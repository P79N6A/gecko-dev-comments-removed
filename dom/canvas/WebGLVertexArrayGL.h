




#ifndef WEBGL_VERTEX_ARRAY_GL_H_
#define WEBGL_VERTEX_ARRAY_GL_H_

#include "WebGLVertexArray.h"

namespace mozilla {

class WebGLVertexArrayGL MOZ_FINAL
    : public WebGLVertexArray
{
public:
    virtual void DeleteImpl() MOZ_OVERRIDE;
    virtual void BindVertexArrayImpl() MOZ_OVERRIDE;
    virtual void GenVertexArray() MOZ_OVERRIDE;

private:
    explicit WebGLVertexArrayGL(WebGLContext* webgl)
        : WebGLVertexArray(webgl)
    { }

    ~WebGLVertexArrayGL() {
        DeleteOnce();
    }

    friend class WebGLVertexArray;
};

} 

#endif 
