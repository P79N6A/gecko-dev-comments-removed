




#include "WebGLVertexArrayGL.h"

#include "WebGLContext.h"
#include "GLContext.h"

namespace mozilla {

void
WebGLVertexArrayGL::DeleteImpl()
{
    mElementArrayBuffer = nullptr;

    mContext->MakeContextCurrent();
    mContext->gl->fDeleteVertexArrays(1, &mGLName);
}

void
WebGLVertexArrayGL::BindVertexArrayImpl()
{
    mContext->mBoundVertexArray = this;

    mContext->gl->fBindVertexArray(mGLName);
}

void
WebGLVertexArrayGL::GenVertexArray()
{
    mContext->gl->fGenVertexArrays(1, &mGLName);
}

} 
