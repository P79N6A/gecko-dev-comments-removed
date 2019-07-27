




#include "WebGLContext.h"
#include "WebGLTexture.h"
#include "WebGLRenderbuffer.h"
#include "WebGLFramebuffer.h"
#include "GLContext.h"

using namespace mozilla;

void
WebGLContext::Clear(GLbitfield mask)
{
    if (IsContextLost())
        return;

    MakeContextCurrent();

    uint32_t m = mask & (LOCAL_GL_COLOR_BUFFER_BIT | LOCAL_GL_DEPTH_BUFFER_BIT | LOCAL_GL_STENCIL_BUFFER_BIT);
    if (mask != m)
        return ErrorInvalidValue("clear: invalid mask bits");

    if (mask == 0) {
        GenerateWarning("Calling gl.clear(0) has no effect.");
    } else if (mRasterizerDiscardEnabled) {
        GenerateWarning("Calling gl.clear() with RASTERIZER_DISCARD enabled has no effects.");
    }

    if (mBoundDrawFramebuffer) {
        if (!mBoundDrawFramebuffer->CheckAndInitializeAttachments())
            return ErrorInvalidFramebufferOperation("clear: incomplete framebuffer");

        gl->fClear(mask);
        return;
    } else {
        ClearBackbufferIfNeeded();
    }

    
    {
        ScopedMaskWorkaround autoMask(*this);
        gl->fClear(mask);
    }

    Invalidate();
    mShouldPresent = true;
}

static GLfloat
GLClampFloat(GLfloat val)
{
    if (val < 0.0)
        return 0.0;

    if (val > 1.0)
        return 1.0;

    return val;
}

void
WebGLContext::ClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
    if (IsContextLost())
        return;

    MakeContextCurrent();

    const bool supportsFloatColorBuffers = (IsExtensionEnabled(WebGLExtensionID::EXT_color_buffer_half_float) ||
                                            IsExtensionEnabled(WebGLExtensionID::WEBGL_color_buffer_float));
    if (!supportsFloatColorBuffers) {
        r = GLClampFloat(r);
        g = GLClampFloat(g);
        b = GLClampFloat(b);
        a = GLClampFloat(a);
    }

    gl->fClearColor(r, g, b, a);

    mColorClearValue[0] = r;
    mColorClearValue[1] = g;
    mColorClearValue[2] = b;
    mColorClearValue[3] = a;
}

void
WebGLContext::ClearDepth(GLclampf v)
{
    if (IsContextLost())
        return;

    MakeContextCurrent();
    mDepthClearValue = GLClampFloat(v);
    gl->fClearDepth(v);
}

void
WebGLContext::ClearStencil(GLint v)
{
    if (IsContextLost())
        return;

    MakeContextCurrent();
    mStencilClearValue = v;
    gl->fClearStencil(v);
}

void
WebGLContext::ColorMask(WebGLboolean r, WebGLboolean g, WebGLboolean b, WebGLboolean a)
{
    if (IsContextLost())
        return;

    MakeContextCurrent();
    mColorWriteMask[0] = r;
    mColorWriteMask[1] = g;
    mColorWriteMask[2] = b;
    mColorWriteMask[3] = a;
    gl->fColorMask(r, g, b, a);
}

void
WebGLContext::DepthMask(WebGLboolean b)
{
    if (IsContextLost())
        return;

    MakeContextCurrent();
    mDepthWriteMask = b;
    gl->fDepthMask(b);
}

void
WebGLContext::DrawBuffers(const dom::Sequence<GLenum>& buffers)
{
    if (IsContextLost())
        return;

    const size_t buffersLength = buffers.Length();

    if (!buffersLength) {
        return ErrorInvalidValue("drawBuffers: invalid <buffers> (buffers must not be empty)");
    }

    if (!mBoundDrawFramebuffer) {
        

        







        if (buffersLength != 1) {
            return ErrorInvalidValue("drawBuffers: invalid <buffers> (main framebuffer: buffers.length must be 1)");
        }

        MakeContextCurrent();

        if (buffers[0] == LOCAL_GL_NONE) {
            const GLenum drawBuffersCommand = LOCAL_GL_NONE;
            gl->fDrawBuffers(1, &drawBuffersCommand);
            return;
        }
        else if (buffers[0] == LOCAL_GL_BACK) {
            const GLenum drawBuffersCommand = LOCAL_GL_COLOR_ATTACHMENT0;
            gl->fDrawBuffers(1, &drawBuffersCommand);
            return;
        }
        return ErrorInvalidOperation("drawBuffers: invalid operation (main framebuffer: buffers[0] must be GL_NONE or GL_BACK)");
    }

    

    if (buffersLength > size_t(mGLMaxDrawBuffers)) {
        






        return ErrorInvalidValue("drawBuffers: invalid <buffers> (buffers.length > GL_MAX_DRAW_BUFFERS)");
    }

    for (uint32_t i = 0; i < buffersLength; i++)
    {
        






        


        if (buffers[i] != LOCAL_GL_NONE &&
            buffers[i] != GLenum(LOCAL_GL_COLOR_ATTACHMENT0 + i)) {
            return ErrorInvalidOperation("drawBuffers: invalid operation (buffers[i] must be GL_NONE or GL_COLOR_ATTACHMENTi)");
        }
    }

    MakeContextCurrent();

    gl->fDrawBuffers(buffersLength, buffers.Elements());
}

void
WebGLContext::StencilMask(GLuint mask)
{
    if (IsContextLost())
        return;

    mStencilWriteMaskFront = mask;
    mStencilWriteMaskBack = mask;

    MakeContextCurrent();
    gl->fStencilMask(mask);
}

void
WebGLContext::StencilMaskSeparate(GLenum face, GLuint mask)
{
    if (IsContextLost())
        return;

    if (!ValidateFaceEnum(face, "stencilMaskSeparate: face"))
        return;

    switch (face) {
        case LOCAL_GL_FRONT_AND_BACK:
            mStencilWriteMaskFront = mask;
            mStencilWriteMaskBack = mask;
            break;
        case LOCAL_GL_FRONT:
            mStencilWriteMaskFront = mask;
            break;
        case LOCAL_GL_BACK:
            mStencilWriteMaskBack = mask;
            break;
    }

    MakeContextCurrent();
    gl->fStencilMaskSeparate(face, mask);
}




