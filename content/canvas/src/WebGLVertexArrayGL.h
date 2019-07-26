




#ifndef WEBGLVERTEXARRAYGL_H_
#define WEBGLVERTEXARRAYGL_H_

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
    WebGLVertexArrayGL(WebGLContext* context)
        : WebGLVertexArray(context)
    { }

    ~WebGLVertexArrayGL() {
        DeleteOnce();
    }

    friend class WebGLVertexArray;
};

} 

#endif
