




#ifndef WEBGL_VERTEX_ARRAY_GL_H_
#define WEBGL_VERTEX_ARRAY_GL_H_

#include "WebGLVertexArray.h"

namespace mozilla {

class WebGLVertexArrayGL
    : public WebGLVertexArray
{
    friend class WebGLVertexArray;

public:
    virtual void DeleteImpl() override;
    virtual void BindVertexArrayImpl() override;
    virtual void GenVertexArray() override;

protected:
    explicit WebGLVertexArrayGL(WebGLContext* webgl)
        : WebGLVertexArray(webgl)
    { }

    ~WebGLVertexArrayGL() {
        DeleteOnce();
    }
};

} 

#endif 
