




#ifndef WEBGL_VERTEX_ARRAY_FAKE_H_
#define WEBGL_VERTEX_ARRAY_FAKE_H_

#include "WebGLVertexArray.h"

namespace mozilla {

class WebGLVertexArrayFake MOZ_FINAL
    : public WebGLVertexArray
{
public:
    virtual void BindVertexArrayImpl() MOZ_OVERRIDE;
    virtual void DeleteImpl() MOZ_OVERRIDE {};
    virtual void GenVertexArray() MOZ_OVERRIDE {};

private:
    explicit WebGLVertexArrayFake(WebGLContext* webgl)
        : WebGLVertexArray(webgl)
    { }

    ~WebGLVertexArrayFake() {
        DeleteOnce();
    }

    friend class WebGLVertexArray;
};

} 

#endif 
