




#include "WebGLVertexArrayFake.h"

#include "GLContext.h"
#include "WebGLContext.h"

namespace mozilla {

WebGLVertexArrayFake::WebGLVertexArrayFake(WebGLContext* webgl)
    : WebGLVertexArray(webgl)
    , mIsVAO(false)
{ }

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

        if (vd.integer) {
            gl->fVertexAttribIPointer(i, vd.size, vd.type, vd.stride,
                                      reinterpret_cast<const GLvoid*>(vd.byteOffset));
        } else {
            gl->fVertexAttribPointer(i, vd.size, vd.type, vd.normalized, vd.stride,
                                     reinterpret_cast<const GLvoid*>(vd.byteOffset));
        }

        if (vd.enabled)
            gl->fEnableVertexAttribArray(i);
        else
            gl->fDisableVertexAttribArray(i);
    }

    size_t len = prevVertexArray->mAttribs.Length();
    for (size_t i = mAttribs.Length(); i < len; ++i) {
        const WebGLVertexAttribData& vd = prevVertexArray->mAttribs[i];

        if (vd.enabled)
            gl->fDisableVertexAttribArray(i);
    }

    mContext->BindBuffer(LOCAL_GL_ARRAY_BUFFER, prevBuffer);
    mIsVAO = true;
}

void
WebGLVertexArrayFake::DeleteImpl()
{
    mIsVAO = false;
}

bool
WebGLVertexArrayFake::IsVertexArrayImpl()
{
    return mIsVAO;
}

} 
