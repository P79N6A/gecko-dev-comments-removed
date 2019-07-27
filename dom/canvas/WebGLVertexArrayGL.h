




#ifndef WEBGL_VERTEX_ARRAY_GL_H_
#define WEBGL_VERTEX_ARRAY_GL_H_

#include "WebGLVertexArray.h"

namespace mozilla {

class WebGLVertexArrayGL final
    : public WebGLVertexArray
{
public:
    virtual void DeleteImpl() override;
    virtual void BindVertexArrayImpl() override;
    virtual void GenVertexArray() override;

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
