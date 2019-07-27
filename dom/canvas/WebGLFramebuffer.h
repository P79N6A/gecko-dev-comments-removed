




#ifndef WEBGL_FRAMEBUFFER_H_
#define WEBGL_FRAMEBUFFER_H_

#include "mozilla/LinkedList.h"
#include "mozilla/WeakPtr.h"
#include "nsWrapperCache.h"

#include "WebGLObjectModel.h"
#include "WebGLStrongTypes.h"
#include "WebGLRenderbuffer.h"
#include "WebGLTexture.h"

namespace mozilla {

class WebGLFramebuffer;
class WebGLRenderbuffer;
class WebGLTexture;

namespace gl {
    class GLContext;
} 

class WebGLFBAttachPoint
{
public:
    WebGLFramebuffer* const mFB;
private:
    WebGLRefPtr<WebGLTexture> mTexturePtr;
    WebGLRefPtr<WebGLRenderbuffer> mRenderbufferPtr;
    FBAttachment mAttachmentPoint;
    TexImageTarget mTexImageTarget;
    GLint mTexImageLevel;

public:
    WebGLFBAttachPoint(WebGLFramebuffer* fb, FBAttachment attachmentPoint);
    ~WebGLFBAttachPoint();

    void Unlink() {
        mRenderbufferPtr = nullptr;
        mTexturePtr = nullptr;
    }

    bool IsDefined() const;
    bool IsDeleteRequested() const;

    TexInternalFormat EffectiveInternalFormat() const;

    bool HasAlpha() const;
    bool IsReadableFloat() const;

    void Clear() {
        SetRenderbuffer(nullptr);
    }

    void SetTexImage(WebGLTexture* tex, TexImageTarget target, GLint level);
    void SetRenderbuffer(WebGLRenderbuffer* rb);

    const WebGLTexture* Texture() const {
        return mTexturePtr;
    }
    WebGLTexture* Texture() {
        return mTexturePtr;
    }
    const WebGLRenderbuffer* Renderbuffer() const {
        return mRenderbufferPtr;
    }
    WebGLRenderbuffer* Renderbuffer() {
        return mRenderbufferPtr;
    }
    TexImageTarget ImageTarget() const {
        return mTexImageTarget;
    }
    GLint MipLevel() const {
        return mTexImageLevel;
    }

    bool HasUninitializedImageData() const;
    void SetImageDataStatus(WebGLImageDataStatus x);

    const WebGLRectangleObject& RectangleObject() const;

    bool HasImage() const;
    bool IsComplete() const;

    void FinalizeAttachment(gl::GLContext* gl,
                            FBAttachment attachmentLoc) const;
};

class WebGLFramebuffer final
    : public nsWrapperCache
    , public WebGLRefCountedObject<WebGLFramebuffer>
    , public LinkedListElement<WebGLFramebuffer>
    , public WebGLContextBoundObject
    , public SupportsWeakPtr<WebGLFramebuffer>
{
    friend class WebGLContext;

public:
    MOZ_DECLARE_WEAKREFERENCE_TYPENAME(WebGLFramebuffer)

    const GLuint mGLName;

private:
    mutable GLenum mStatus;

    GLenum mReadBufferMode;

    
    WebGLFBAttachPoint mColorAttachment0;
    WebGLFBAttachPoint mDepthAttachment;
    WebGLFBAttachPoint mStencilAttachment;
    WebGLFBAttachPoint mDepthStencilAttachment;
    nsTArray<WebGLFBAttachPoint> mMoreColorAttachments;

#ifdef ANDROID
    
    
    
    
    
    bool mIsFB;
#endif

public:
    WebGLFramebuffer(WebGLContext* webgl, GLuint fbo);

private:
    ~WebGLFramebuffer() {
        DeleteOnce();
    }

    const WebGLRectangleObject& GetAnyRectObject() const;

public:
    void Delete();

    void FramebufferRenderbuffer(FBAttachment attachment, RBTarget rbtarget,
                                 WebGLRenderbuffer* rb);

    void FramebufferTexture2D(FBAttachment attachment,
                              TexImageTarget texImageTarget, WebGLTexture* tex,
                              GLint level);

    bool HasDefinedAttachments() const;
    bool HasIncompleteAttachments() const;
    bool AllImageRectsMatch() const;
    FBStatus PrecheckFramebufferStatus() const;
    FBStatus CheckFramebufferStatus() const;

    GLenum
    GetFormatForAttachment(const WebGLFBAttachPoint& attachment) const;

    bool HasDepthStencilConflict() const {
        return int(mDepthAttachment.IsDefined()) +
               int(mStencilAttachment.IsDefined()) +
               int(mDepthStencilAttachment.IsDefined()) >= 2;
    }

    size_t ColorAttachmentCount() const {
        return 1 + mMoreColorAttachments.Length();
    }
    const WebGLFBAttachPoint& ColorAttachment(size_t colorAttachmentId) const {
        MOZ_ASSERT(colorAttachmentId < ColorAttachmentCount());
        return colorAttachmentId ? mMoreColorAttachments[colorAttachmentId - 1]
                                 : mColorAttachment0;
    }

    const WebGLFBAttachPoint& DepthAttachment() const {
        return mDepthAttachment;
    }

    const WebGLFBAttachPoint& StencilAttachment() const {
        return mStencilAttachment;
    }

    const WebGLFBAttachPoint& DepthStencilAttachment() const {
        return mDepthStencilAttachment;
    }

    WebGLFBAttachPoint& GetAttachPoint(FBAttachment attachPointEnum);

    void DetachTexture(const WebGLTexture* tex);

    void DetachRenderbuffer(const WebGLRenderbuffer* rb);

    const WebGLRectangleObject& RectangleObject() const;

    WebGLContext* GetParentObject() const {
        return Context();
    }

    void FinalizeAttachments() const;

    virtual JSObject* WrapObject(JSContext* cx, JS::Handle<JSObject*> givenProto) override;

    NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(WebGLFramebuffer)
    NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(WebGLFramebuffer)

    
    bool HasCompletePlanes(GLbitfield mask);

    bool CheckAndInitializeAttachments();

    bool CheckColorAttachmentNumber(FBAttachment attachment,
                                    const char* funcName) const;

    void EnsureColorAttachPoints(size_t colorAttachmentId);

    void InvalidateFramebufferStatus() const {
        mStatus = 0;
    }

    bool ValidateForRead(const char* info, TexInternalFormat* const out_format);
};

} 

#endif 
