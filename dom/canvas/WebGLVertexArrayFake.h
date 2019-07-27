




#ifndef WEBGL_VERTEX_ARRAY_FAKE_H_
#define WEBGL_VERTEX_ARRAY_FAKE_H_

#include "WebGLVertexArray.h"

namespace mozilla {

class WebGLVertexArrayFake final
    : public WebGLVertexArray
{
    friend class WebGLVertexArray;

protected:
    virtual void BindVertexArrayImpl() override;
    virtual void DeleteImpl() override;
    virtual void GenVertexArray() override {};
    virtual bool IsVertexArrayImpl() override;

private:
    explicit WebGLVertexArrayFake(WebGLContext* webgl);

    ~WebGLVertexArrayFake() {
        DeleteOnce();
    }

    bool mIsVAO;
};

} 

#endif 
