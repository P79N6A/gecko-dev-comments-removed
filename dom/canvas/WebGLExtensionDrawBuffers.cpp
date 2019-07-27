




#include "WebGLExtensions.h"

#include <algorithm>
#include "GLContext.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"
#include "WebGLContext.h"
#include "WebGLFramebuffer.h"
#include "WebGLRenderbuffer.h"
#include "WebGLTexture.h"

namespace mozilla {

WebGLExtensionDrawBuffers::WebGLExtensionDrawBuffers(WebGLContext* webgl)
    : WebGLExtensionBase(webgl)
{
    MOZ_ASSERT(IsSupported(webgl), "Don't construct extension if unsupported.");

    GLint maxColorAttachments = 0;
    GLint maxDrawBuffers = 0;

    webgl->MakeContextCurrent();
    gl::GLContext* gl = webgl->GL();

    gl->fGetIntegerv(LOCAL_GL_MAX_COLOR_ATTACHMENTS, &maxColorAttachments);
    gl->fGetIntegerv(LOCAL_GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);

    
    maxColorAttachments = std::min(maxColorAttachments, GLint(WebGLContext::kMaxColorAttachments));

    if (webgl->MinCapabilityMode())
        maxColorAttachments = std::min(maxColorAttachments, GLint(kMinColorAttachments));

    
    maxDrawBuffers = std::min(maxDrawBuffers, GLint(maxColorAttachments));

    webgl->mGLMaxColorAttachments = maxColorAttachments;
    webgl->mGLMaxDrawBuffers = maxDrawBuffers;
}

WebGLExtensionDrawBuffers::~WebGLExtensionDrawBuffers()
{
}

void
WebGLExtensionDrawBuffers::DrawBuffersWEBGL(const dom::Sequence<GLenum>& buffers)
{
    if (mIsLost) {
        mContext->ErrorInvalidOperation("drawBuffersWEBGL: Extension is lost.");
        return;
    }

    mContext->DrawBuffers(buffers);
}

bool
WebGLExtensionDrawBuffers::IsSupported(const WebGLContext* webgl)
{
    gl::GLContext* gl = webgl->GL();

    if (!gl->IsSupported(gl::GLFeature::draw_buffers))
        return false;

    GLint supportedColorAttachments = 0;
    GLint supportedDrawBuffers = 0;

    webgl->MakeContextCurrent();

    gl->fGetIntegerv(LOCAL_GL_MAX_COLOR_ATTACHMENTS, &supportedColorAttachments);
    gl->fGetIntegerv(LOCAL_GL_MAX_COLOR_ATTACHMENTS, &supportedDrawBuffers);

    
    if (size_t(supportedColorAttachments) < kMinColorAttachments)
        return false;

    if (size_t(supportedDrawBuffers) < kMinDrawBuffers)
        return false;

    return true;
}

IMPL_WEBGL_EXTENSION_GOOP(WebGLExtensionDrawBuffers)

} 
