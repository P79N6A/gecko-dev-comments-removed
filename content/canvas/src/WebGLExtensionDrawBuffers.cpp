



#include "WebGLContext.h"
#include "WebGLExtensions.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"
#include "WebGLTexture.h"
#include "WebGLRenderbuffer.h"
#include "WebGLFramebuffer.h"

#include <algorithm>

using namespace mozilla;
using namespace gl;

WebGLExtensionDrawBuffers::WebGLExtensionDrawBuffers(WebGLContext* context)
    : WebGLExtensionBase(context)
{
    GLint maxColorAttachments = 0;
    GLint maxDrawBuffers = 0;

    gl::GLContext* gl = context->GL();

    context->MakeContextCurrent();

    gl->fGetIntegerv(LOCAL_GL_MAX_COLOR_ATTACHMENTS, &maxColorAttachments);
    gl->fGetIntegerv(LOCAL_GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);

    
    maxColorAttachments = std::min(maxColorAttachments, GLint(WebGLContext::sMaxColorAttachments));

    if (context->MinCapabilityMode())
    {
        maxColorAttachments = std::min(maxColorAttachments, GLint(sMinColorAttachments));
    }

    
    maxDrawBuffers = std::min(maxDrawBuffers, GLint(maxColorAttachments));

    context->mGLMaxColorAttachments = maxColorAttachments;
    context->mGLMaxDrawBuffers = maxDrawBuffers;
}

WebGLExtensionDrawBuffers::~WebGLExtensionDrawBuffers()
{
}

void WebGLExtensionDrawBuffers::DrawBuffersWEBGL(const dom::Sequence<GLenum>& buffers)
{
    const size_t buffersLength = buffers.Length();

    if (buffersLength == 0) {
        return mContext->ErrorInvalidValue("drawBuffersWEBGL: invalid <buffers> (buffers must not be empty)");
    }

    if (mContext->mBoundFramebuffer == 0)
    {
        

        







        if (buffersLength != 1) {
            return mContext->ErrorInvalidValue("drawBuffersWEBGL: invalid <buffers> (main framebuffer: buffers.length must be 1)");
        }

        mContext->MakeContextCurrent();

        if (buffers[0] == LOCAL_GL_NONE) {
            const GLenum drawBufffersCommand = LOCAL_GL_NONE;
            mContext->GL()->fDrawBuffers(1, &drawBufffersCommand);
            return;
        }
        else if (buffers[0] == LOCAL_GL_BACK) {
            const GLenum drawBufffersCommand = LOCAL_GL_COLOR_ATTACHMENT0;
            mContext->GL()->fDrawBuffers(1, &drawBufffersCommand);
            return;
        }
        return mContext->ErrorInvalidOperation("drawBuffersWEBGL: invalid operation (main framebuffer: buffers[0] must be GL_NONE or GL_BACK)");
    }

    

    if (buffersLength > size_t(mContext->mGLMaxDrawBuffers)) {
        






        return mContext->ErrorInvalidValue("drawBuffersWEBGL: invalid <buffers> (buffers.length > GL_MAX_DRAW_BUFFERS)");
    }

    for (uint32_t i = 0; i < buffersLength; i++)
    {
        






        


        if (buffers[i] != LOCAL_GL_NONE &&
            buffers[i] != GLenum(LOCAL_GL_COLOR_ATTACHMENT0 + i)) {
            return mContext->ErrorInvalidOperation("drawBuffersWEBGL: invalid operation (buffers[i] must be GL_NONE or GL_COLOR_ATTACHMENTi)");
        }
    }

    mContext->MakeContextCurrent();

    mContext->GL()->fDrawBuffers(buffersLength, buffers.Elements());
}

bool WebGLExtensionDrawBuffers::IsSupported(const WebGLContext* context)
{
    gl::GLContext * gl = context->GL();

    if (!gl->IsExtensionSupported(gl->IsGLES2() ? GLContext::EXT_draw_buffers
                                                : GLContext::ARB_draw_buffers)) {
        return false;
    }

    GLint supportedColorAttachments = 0;
    GLint supportedDrawBuffers = 0;

    context->MakeContextCurrent();

    gl->fGetIntegerv(LOCAL_GL_MAX_COLOR_ATTACHMENTS, &supportedColorAttachments);
    gl->fGetIntegerv(LOCAL_GL_MAX_COLOR_ATTACHMENTS, &supportedDrawBuffers);

    if (size_t(supportedColorAttachments) < sMinColorAttachments){
        
        return false;
    }

    if (size_t(supportedDrawBuffers) < sMinDrawBuffers){
        return false;
    }

    return true;
}

IMPL_WEBGL_EXTENSION_GOOP(WebGLExtensionDrawBuffers)
