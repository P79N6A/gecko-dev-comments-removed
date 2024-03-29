




#include "WebGLRenderbuffer.h"

#include "GLContext.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"
#include "ScopedGLHelpers.h"
#include "WebGLContext.h"
#include "WebGLStrongTypes.h"
#include "WebGLTexture.h"

namespace mozilla {

static GLenum
DepthStencilDepthFormat(gl::GLContext* gl)
{
    
    if (gl->IsGLES() && !gl->IsExtensionSupported(gl::GLContext::OES_depth24))
        return LOCAL_GL_DEPTH_COMPONENT16;

    return LOCAL_GL_DEPTH_COMPONENT24;
}

static bool
SupportsDepthStencil(gl::GLContext* gl)
{
    return gl->IsExtensionSupported(gl::GLContext::EXT_packed_depth_stencil) ||
           gl->IsExtensionSupported(gl::GLContext::OES_packed_depth_stencil);
}

static bool
NeedsDepthStencilEmu(gl::GLContext* gl, GLenum internalFormat)
{
    MOZ_ASSERT(internalFormat != LOCAL_GL_DEPTH_STENCIL);
    if (internalFormat != LOCAL_GL_DEPTH24_STENCIL8)
        return false;

    return !SupportsDepthStencil(gl);
}

JSObject*
WebGLRenderbuffer::WrapObject(JSContext* cx, JS::Handle<JSObject*> givenProto)
{
    return dom::WebGLRenderbufferBinding::Wrap(cx, this, givenProto);
}

WebGLRenderbuffer::WebGLRenderbuffer(WebGLContext* webgl)
    : WebGLContextBoundObject(webgl)
    , mPrimaryRB(0)
    , mSecondaryRB(0)
    , mInternalFormat(0)
    , mInternalFormatForGL(0)
    , mImageDataStatus(WebGLImageDataStatus::NoImageData)
    , mSamples(1)
#ifdef ANDROID
    , mIsRB(false)
#endif
{
    mContext->MakeContextCurrent();

    mContext->gl->fGenRenderbuffers(1, &mPrimaryRB);
    if (!SupportsDepthStencil(mContext->gl))
        mContext->gl->fGenRenderbuffers(1, &mSecondaryRB);

    mContext->mRenderbuffers.insertBack(this);
}

void
WebGLRenderbuffer::Delete()
{
    mContext->MakeContextCurrent();

    mContext->gl->fDeleteRenderbuffers(1, &mPrimaryRB);
    if (mSecondaryRB)
        mContext->gl->fDeleteRenderbuffers(1, &mSecondaryRB);

    LinkedListElement<WebGLRenderbuffer>::removeFrom(mContext->mRenderbuffers);
#ifdef ANDROID
    mIsRB = false;
#endif
}

int64_t
WebGLRenderbuffer::MemoryUsage() const
{
    int64_t pixels = int64_t(Width()) * int64_t(Height());

    GLenum primaryFormat = InternalFormatForGL();
    
    if (!primaryFormat)
        return 0;

    int64_t secondarySize = 0;
    if (mSecondaryRB) {
        if (NeedsDepthStencilEmu(mContext->gl, primaryFormat)) {
            primaryFormat = DepthStencilDepthFormat(mContext->gl);
            secondarySize = 1*pixels; 
        } else {
            secondarySize = 1*1*2; 
        }
    }

    int64_t primarySize = 0;
    switch (primaryFormat) {
        case LOCAL_GL_STENCIL_INDEX8:
            primarySize = 1*pixels;
            break;
        case LOCAL_GL_RGBA4:
        case LOCAL_GL_RGB5_A1:
        case LOCAL_GL_RGB565:
        case LOCAL_GL_DEPTH_COMPONENT16:
            primarySize = 2*pixels;
            break;
        case LOCAL_GL_RGB8:
        case LOCAL_GL_DEPTH_COMPONENT24:
            primarySize = 3*pixels;
            break;
        case LOCAL_GL_RGBA8:
        case LOCAL_GL_SRGB8_ALPHA8_EXT:
        case LOCAL_GL_DEPTH24_STENCIL8:
        case LOCAL_GL_DEPTH_COMPONENT32:
            primarySize = 4*pixels;
            break;
        case LOCAL_GL_RGB16F:
            primarySize = 2*3*pixels;
            break;
        case LOCAL_GL_RGBA16F:
            primarySize = 2*4*pixels;
            break;
        case LOCAL_GL_RGB32F:
            primarySize = 4*3*pixels;
            break;
        case LOCAL_GL_RGBA32F:
            primarySize = 4*4*pixels;
            break;
        default:
            MOZ_ASSERT(false, "Unknown `primaryFormat`.");
            break;
    }

    return primarySize + secondarySize;
}

void
WebGLRenderbuffer::BindRenderbuffer() const
{
    












    mContext->gl->fBindRenderbuffer(LOCAL_GL_RENDERBUFFER, mPrimaryRB);
}

static void
RenderbufferStorageMaybeMultisample(gl::GLContext* gl, GLsizei samples,
                                    GLenum internalFormat, GLsizei width,
                                    GLsizei height)
{
    MOZ_ASSERT_IF(samples >= 1, gl->IsSupported(gl::GLFeature::framebuffer_multisample));
    MOZ_ASSERT(samples >= 0);
    MOZ_ASSERT(samples <= gl->MaxSamples());

    if (samples > 0) {
        gl->fRenderbufferStorageMultisample(LOCAL_GL_RENDERBUFFER, samples,
                                            internalFormat, width, height);
    } else {
        gl->fRenderbufferStorage(LOCAL_GL_RENDERBUFFER, internalFormat, width,
                                 height);
    }
}

void
WebGLRenderbuffer::RenderbufferStorage(GLsizei samples, GLenum internalFormat,
                                       GLsizei width, GLsizei height) const
{
    MOZ_ASSERT(mContext->mBoundRenderbuffer == this);

    InvalidateStatusOfAttachedFBs();

    gl::GLContext* gl = mContext->gl;
    MOZ_ASSERT(samples >= 0 && samples <= 256); 

    GLenum primaryFormat = internalFormat;
    GLenum secondaryFormat = 0;

    if (NeedsDepthStencilEmu(mContext->gl, primaryFormat)) {
        primaryFormat = DepthStencilDepthFormat(gl);
        secondaryFormat = LOCAL_GL_STENCIL_INDEX8;
    }

    RenderbufferStorageMaybeMultisample(gl, samples, primaryFormat, width,
                                        height);

    if (!mSecondaryRB) {
        MOZ_ASSERT(!secondaryFormat);
        return;
    }
    
    
    
    
    gl::ScopedBindRenderbuffer autoRB(gl, mSecondaryRB);
    if (secondaryFormat) {
        RenderbufferStorageMaybeMultisample(gl, samples, secondaryFormat, width,
                                            height);
    } else {
        RenderbufferStorageMaybeMultisample(gl, samples, LOCAL_GL_RGBA4, 1, 1);
    }
}

void
WebGLRenderbuffer::FramebufferRenderbuffer(FBAttachment attachment) const
{
    gl::GLContext* gl = mContext->gl;
    if (attachment != LOCAL_GL_DEPTH_STENCIL_ATTACHMENT) {
        gl->fFramebufferRenderbuffer(LOCAL_GL_FRAMEBUFFER, attachment.get(),
                                     LOCAL_GL_RENDERBUFFER, mPrimaryRB);
        return;
    }

    GLuint stencilRB = mPrimaryRB;
    if (NeedsDepthStencilEmu(mContext->gl, InternalFormatForGL())) {
        MOZ_ASSERT(mSecondaryRB);
        stencilRB = mSecondaryRB;
    }
    gl->fFramebufferRenderbuffer(LOCAL_GL_FRAMEBUFFER,
                                 LOCAL_GL_DEPTH_ATTACHMENT,
                                 LOCAL_GL_RENDERBUFFER, mPrimaryRB);
    gl->fFramebufferRenderbuffer(LOCAL_GL_FRAMEBUFFER,
                                 LOCAL_GL_STENCIL_ATTACHMENT,
                                 LOCAL_GL_RENDERBUFFER, stencilRB);
}

GLint
WebGLRenderbuffer::GetRenderbufferParameter(RBTarget target,
                                            RBParam pname) const
{
    gl::GLContext* gl = mContext->gl;

    switch (pname.get()) {
    case LOCAL_GL_RENDERBUFFER_STENCIL_SIZE:
        if (NeedsDepthStencilEmu(mContext->gl, InternalFormatForGL())) {
            if (gl->WorkAroundDriverBugs() &&
                gl->Renderer() == gl::GLRenderer::Tegra)
            {
                return 8;
            }

            gl::ScopedBindRenderbuffer autoRB(gl, mSecondaryRB);

            GLint i = 0;
            gl->fGetRenderbufferParameteriv(target.get(), pname.get(), &i);
            return i;
        }
        

    case LOCAL_GL_RENDERBUFFER_WIDTH:
    case LOCAL_GL_RENDERBUFFER_HEIGHT:
    case LOCAL_GL_RENDERBUFFER_RED_SIZE:
    case LOCAL_GL_RENDERBUFFER_GREEN_SIZE:
    case LOCAL_GL_RENDERBUFFER_BLUE_SIZE:
    case LOCAL_GL_RENDERBUFFER_ALPHA_SIZE:
    case LOCAL_GL_RENDERBUFFER_DEPTH_SIZE:
        {
            GLint i = 0;
            gl->fGetRenderbufferParameteriv(target.get(), pname.get(), &i);
            return i;
        }
    }

    MOZ_ASSERT(false,
               "This function should only be called with valid `pname`.");
    return 0;
}

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(WebGLRenderbuffer)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(WebGLRenderbuffer, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(WebGLRenderbuffer, Release)

} 
