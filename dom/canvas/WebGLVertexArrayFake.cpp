




#include "WebGLVertexArrayFake.h"

#include "WebGLContext.h"
#include "GLContext.h"

namespace mozilla {

void
WebGLVertexArrayFake::BindVertexArrayImpl()
{
    
    
    gl::GLContext* gl = mContext->gl;

    WebGLRefPtr<WebGLVertexArray> prevVertexArray = mContext->mBoundVertexArray;

    mContext->mBoundVertexArray = this;

    WebGLRefPtr<WebGLBuffer> prevBuffer = mContext->mBoundArrayBuffer;
    mContext->BindBuffer(LOCAL_GL_ELEMENT_ARRAY_BUFFER, mElementArrayBuffer);

    for (size_t i = 0; i < mAttribs.Length(); ++i) {
        const WebGLVertexAttribData& vd = mAttribs[i];

        mContext->BindBuffer(LOCAL_GL_ARRAY_BUFFER, vd.buf);

        gl->fVertexAttribPointer(i, vd.size, vd.type, vd.normalized,
                                 vd.stride, reinterpret_cast<void*>(vd.byteOffset));

        if (vd.enabled) {
            gl->fEnableVertexAttribArray(i);
        } else {
            gl->fDisableVertexAttribArray(i);
        }
    }

    for (size_t i = mAttribs.Length(); i < prevVertexArray->mAttribs.Length(); ++i) {
        const WebGLVertexAttribData& vd = prevVertexArray->mAttribs[i];

        if (vd.enabled) {
            gl->fDisableVertexAttribArray(i);
        }
    }

    mContext->BindBuffer(LOCAL_GL_ARRAY_BUFFER, prevBuffer);
}

} 

