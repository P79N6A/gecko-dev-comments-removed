




#ifndef WEBGLVERTEXARRAYFAKE_H_
#define WEBGLVERTEXARRAYFAKE_H_

#include "WebGLVertexArray.h"

namespace mozilla {

class WebGLVertexArrayFake MOZ_FINAL
    : public WebGLVertexArray
{
public:
    virtual void BindVertexArrayImpl() MOZ_OVERRIDE;
    virtual void DeleteImpl() MOZ_OVERRIDE { };
    virtual void GenVertexArray() MOZ_OVERRIDE { };

private:
    WebGLVertexArrayFake(WebGLContext *context)
        : WebGLVertexArray(context)
    { }

    ~WebGLVertexArrayFake() {
        DeleteOnce();
    }

    friend class WebGLVertexArray;
};

} 

#endif
