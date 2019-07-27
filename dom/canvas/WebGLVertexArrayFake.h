




#ifndef WEBGL_VERTEX_ARRAY_FAKE_H_
#define WEBGL_VERTEX_ARRAY_FAKE_H_

#include "WebGLVertexArray.h"

namespace mozilla {

class WebGLVertexArrayFake final
    : public WebGLVertexArray
{
public:
    virtual void BindVertexArrayImpl() override;
    virtual void DeleteImpl() override {};
    virtual void GenVertexArray() override {};

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
