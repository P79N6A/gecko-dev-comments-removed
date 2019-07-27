




#ifndef WEBGL_FRAMEBUFFER_H_
#define WEBGL_FRAMEBUFFER_H_

#include "mozilla/LinkedList.h"
#include "nsWrapperCache.h"
#include "WebGLBindableName.h"
#include "WebGLObjectModel.h"
#include "WebGLStrongTypes.h"

namespace mozilla {

class WebGLFramebufferAttachable;
class WebGLRenderbuffer;
class WebGLTexture;

namespace gl {
    class GLContext;
}

class WebGLFramebuffer final
    : public nsWrapperCache
    , public WebGLBindableName<FBTarget>
    , public WebGLRefCountedObject<WebGLFramebuffer>
    , public LinkedListElement<WebGLFramebuffer>
    , public WebGLContextBoundObject
    , public SupportsWeakPtr<WebGLFramebuffer>
{
public:
    MOZ_DECLARE_REFCOUNTED_TYPENAME(WebGLFramebuffer)

    explicit WebGLFramebuffer(WebGLContext* webgl, GLuint fbo);

    struct Attachment
    {
        
        WebGLRefPtr<WebGLTexture> mTexturePtr;
        WebGLRefPtr<WebGLRenderbuffer> mRenderbufferPtr;
        FBAttachment mAttachmentPoint;
        TexImageTarget mTexImageTarget;
        GLint mTexImageLevel;
        mutable bool mNeedsFinalize;

        explicit Attachment(FBAttachment attachmentPoint = LOCAL_GL_COLOR_ATTACHMENT0);
        ~Attachment();

        bool IsDefined() const;

        bool IsDeleteRequested() const;

        TexInternalFormat EffectiveInternalFormat() const;

        bool HasAlpha() const;
        bool IsReadableFloat() const;

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

        void Reset();

        const WebGLRectangleObject& RectangleObject() const;

        bool HasImage() const;
        bool IsComplete() const;

        void FinalizeAttachment(gl::GLContext* gl,
                                FBAttachment attachmentLoc) const;
    };

    void Delete();

    void FramebufferRenderbuffer(FBAttachment attachment, RBTarget rbtarget,
                                 WebGLRenderbuffer* rb);

    void FramebufferTexture2D(FBAttachment attachment,
                              TexImageTarget texImageTarget, WebGLTexture* tex,
                              GLint level);

private:
    void DetachAttachment(WebGLFramebuffer::Attachment& attachment);
    void DetachAllAttachments();
    const WebGLRectangleObject& GetAnyRectObject() const;
    Attachment* GetAttachmentOrNull(FBAttachment attachment);

public:
    bool HasDefinedAttachments() const;
    bool HasIncompleteAttachments() const;
    bool AllImageRectsMatch() const;
    FBStatus PrecheckFramebufferStatus() const;
    FBStatus CheckFramebufferStatus() const;

    GLenum
    GetFormatForAttachment(const WebGLFramebuffer::Attachment& attachment) const;

    bool HasDepthStencilConflict() const {
        return int(mDepthAttachment.IsDefined()) +
               int(mStencilAttachment.IsDefined()) +
               int(mDepthStencilAttachment.IsDefined()) >= 2;
    }

    size_t ColorAttachmentCount() const {
        return mColorAttachments.Length();
    }
    const Attachment& ColorAttachment(size_t colorAttachmentId) const {
        return mColorAttachments[colorAttachmentId];
    }

    const Attachment& DepthAttachment() const {
        return mDepthAttachment;
    }

    const Attachment& StencilAttachment() const {
        return mStencilAttachment;
    }

    const Attachment& DepthStencilAttachment() const {
        return mDepthStencilAttachment;
    }

    const Attachment& GetAttachment(FBAttachment attachment) const;

    void DetachTexture(const WebGLTexture* tex);

    void DetachRenderbuffer(const WebGLRenderbuffer* rb);

    const WebGLRectangleObject& RectangleObject() const;

    WebGLContext* GetParentObject() const {
        return Context();
    }

    void FinalizeAttachments() const;

    virtual JSObject* WrapObject(JSContext* cx, JS::Handle<JSObject*> aGivenProto) override;

    NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(WebGLFramebuffer)
    NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(WebGLFramebuffer)

    
    bool HasCompletePlanes(GLbitfield mask);

    bool CheckAndInitializeAttachments();

    bool CheckColorAttachmentNumber(FBAttachment attachment,
                                    const char* funcName) const;

    void EnsureColorAttachments(size_t colorAttachmentId);

    void NotifyAttachableChanged() const;

    bool ValidateForRead(const char* info, TexInternalFormat* const out_format);

private:
    ~WebGLFramebuffer() {
        DeleteOnce();
    }

    mutable GLenum mStatus;

    
    
    nsTArray<Attachment> mColorAttachments;
    Attachment mDepthAttachment;
    Attachment mStencilAttachment;
    Attachment mDepthStencilAttachment;
    GLenum mReadBufferMode;
};

} 

#endif 
