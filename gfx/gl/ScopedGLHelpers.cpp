




#include "GLContext.h"
#include "ScopedGLHelpers.h"

namespace mozilla {
namespace gl {




ScopedGLState::ScopedGLState(GLContext* aGL, GLenum aCapability, bool aNewState)
    : ScopedGLWrapper<ScopedGLState>(aGL)
    , mCapability(aCapability)
{
    mOldState = mGL->fIsEnabled(mCapability);

    
    if (aNewState == mOldState)
        return;

    if (aNewState) {
        mGL->fEnable(mCapability);
    } else {
        mGL->fDisable(mCapability);
    }
}

void
ScopedGLState::UnwrapImpl()
{
    if (mOldState) {
        mGL->fEnable(mCapability);
    } else {
        mGL->fDisable(mCapability);
    }
}




void
ScopedBindFramebuffer::Init()
{
    mOldFB = mGL->GetFB();
}

ScopedBindFramebuffer::ScopedBindFramebuffer(GLContext* aGL)
    : ScopedGLWrapper<ScopedBindFramebuffer>(aGL)
{
    Init();
}

ScopedBindFramebuffer::ScopedBindFramebuffer(GLContext* aGL, GLuint aNewFB)
    : ScopedGLWrapper<ScopedBindFramebuffer>(aGL)
{
    Init();
    mGL->BindFB(aNewFB);
}

void
ScopedBindFramebuffer::UnwrapImpl()
{
    
    MOZ_ASSERT(mGL->IsCurrent());

    mGL->BindFB(mOldFB);
}




ScopedBindTextureUnit::ScopedBindTextureUnit(GLContext* aGL, GLenum aTexUnit)
    : ScopedGLWrapper<ScopedBindTextureUnit>(aGL)
{
    MOZ_ASSERT(aTexUnit >= LOCAL_GL_TEXTURE0);
    mGL->GetUIntegerv(LOCAL_GL_ACTIVE_TEXTURE, &mOldTexUnit);
    mGL->fActiveTexture(aTexUnit);
}

void
ScopedBindTextureUnit::UnwrapImpl() {
    
    MOZ_ASSERT(mGL->IsCurrent());

    mGL->fActiveTexture(mOldTexUnit);
}




ScopedTexture::ScopedTexture(GLContext* aGL)
    : ScopedGLWrapper<ScopedTexture>(aGL)
{
    mGL->fGenTextures(1, &mTexture);
}

void
ScopedTexture::UnwrapImpl()
{
    
    
    MOZ_ASSERT(mGL->IsCurrent());

    mGL->fDeleteTextures(1, &mTexture);
}


void
ScopedBindTexture::Init(GLenum aTarget)
{
    mTarget = aTarget;
    mOldTex = 0;
    GLenum bindingTarget = (aTarget == LOCAL_GL_TEXTURE_2D) ? LOCAL_GL_TEXTURE_BINDING_2D
                         : (aTarget == LOCAL_GL_TEXTURE_RECTANGLE_ARB) ? LOCAL_GL_TEXTURE_BINDING_RECTANGLE_ARB
                         : (aTarget == LOCAL_GL_TEXTURE_CUBE_MAP) ? LOCAL_GL_TEXTURE_BINDING_CUBE_MAP
                         : LOCAL_GL_NONE;
    MOZ_ASSERT(bindingTarget != LOCAL_GL_NONE);
    mGL->GetUIntegerv(bindingTarget, &mOldTex);
}

ScopedBindTexture::ScopedBindTexture(GLContext* aGL, GLuint aNewTex, GLenum aTarget)
        : ScopedGLWrapper<ScopedBindTexture>(aGL)
    {
        Init(aTarget);
        mGL->fBindTexture(aTarget, aNewTex);
    }

void
ScopedBindTexture::UnwrapImpl()
{
    
    MOZ_ASSERT(mGL->IsCurrent());

    mGL->fBindTexture(mTarget, mOldTex);
}




void
ScopedBindRenderbuffer::Init()
{
    mOldRB = 0;
    mGL->GetUIntegerv(LOCAL_GL_RENDERBUFFER_BINDING, &mOldRB);
}

ScopedBindRenderbuffer::ScopedBindRenderbuffer(GLContext* aGL)
        : ScopedGLWrapper<ScopedBindRenderbuffer>(aGL)
{
    Init();
}

ScopedBindRenderbuffer::ScopedBindRenderbuffer(GLContext* aGL, GLuint aNewRB)
    : ScopedGLWrapper<ScopedBindRenderbuffer>(aGL)
{
    Init();
    mGL->fBindRenderbuffer(LOCAL_GL_RENDERBUFFER, aNewRB);
}

void
ScopedBindRenderbuffer::UnwrapImpl() {
    
    MOZ_ASSERT(mGL->IsCurrent());

    mGL->fBindRenderbuffer(LOCAL_GL_RENDERBUFFER, mOldRB);
}



ScopedFramebufferForTexture::ScopedFramebufferForTexture(GLContext* aGL,
                                                         GLuint aTexture,
                                                         GLenum aTarget)
    : ScopedGLWrapper<ScopedFramebufferForTexture>(aGL)
    , mComplete(false)
    , mFB(0)
{
    mGL->fGenFramebuffers(1, &mFB);
    ScopedBindFramebuffer autoFB(aGL, mFB);
    mGL->fFramebufferTexture2D(LOCAL_GL_FRAMEBUFFER,
                               LOCAL_GL_COLOR_ATTACHMENT0,
                               aTarget,
                               aTexture,
                               0);

    GLenum status = mGL->fCheckFramebufferStatus(LOCAL_GL_FRAMEBUFFER);
    if (status == LOCAL_GL_FRAMEBUFFER_COMPLETE) {
        mComplete = true;
    } else {
        mGL->fDeleteFramebuffers(1, &mFB);
        mFB = 0;
    }
}

void ScopedFramebufferForTexture::UnwrapImpl()
{
    if (!mFB)
        return;

    mGL->fDeleteFramebuffers(1, &mFB);
    mFB = 0;
}





ScopedFramebufferForRenderbuffer::ScopedFramebufferForRenderbuffer(GLContext* aGL,
                                                                   GLuint aRB)
    : ScopedGLWrapper<ScopedFramebufferForRenderbuffer>(aGL)
    , mComplete(false)
    , mFB(0)
{
    mGL->fGenFramebuffers(1, &mFB);
    ScopedBindFramebuffer autoFB(aGL, mFB);
    mGL->fFramebufferRenderbuffer(LOCAL_GL_FRAMEBUFFER,
                                  LOCAL_GL_COLOR_ATTACHMENT0,
                                  LOCAL_GL_RENDERBUFFER,
                                  aRB);

    GLenum status = mGL->fCheckFramebufferStatus(LOCAL_GL_FRAMEBUFFER);
    if (status == LOCAL_GL_FRAMEBUFFER_COMPLETE) {
        mComplete = true;
    } else {
        mGL->fDeleteFramebuffers(1, &mFB);
        mFB = 0;
    }
}

void
ScopedFramebufferForRenderbuffer::UnwrapImpl()
{
    if (!mFB)
        return;

    mGL->fDeleteFramebuffers(1, &mFB);
    mFB = 0;
}



ScopedViewportRect::ScopedViewportRect(GLContext* aGL,
                                       GLint x, GLint y,
                                       GLsizei width, GLsizei height)
  : ScopedGLWrapper<ScopedViewportRect>(aGL)
{
  mGL->fGetIntegerv(LOCAL_GL_VIEWPORT, mSavedViewportRect);
  mGL->fViewport(x, y, width, height);
}

void ScopedViewportRect::UnwrapImpl()
{
  mGL->fViewport(mSavedViewportRect[0],
                 mSavedViewportRect[1],
                 mSavedViewportRect[2],
                 mSavedViewportRect[3]);
}

} 
} 
