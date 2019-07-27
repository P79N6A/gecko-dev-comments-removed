




#ifndef WEBGLFRAMEBUFFER_H_
#define WEBGLFRAMEBUFFER_H_

#include "WebGLBindableName.h"
#include "WebGLObjectModel.h"
#include "WebGLStrongTypes.h"

#include "nsWrapperCache.h"

#include "mozilla/LinkedList.h"

namespace mozilla {

class WebGLFramebufferAttachable;
class WebGLTexture;
class WebGLRenderbuffer;
namespace gl {
    class GLContext;
}

class WebGLFramebuffer MOZ_FINAL
    : public nsWrapperCache
    , public WebGLBindableName<GLenum>
    , public WebGLRefCountedObject<WebGLFramebuffer>
    , public LinkedListElement<WebGLFramebuffer>
    , public WebGLContextBoundObject
    , public SupportsWeakPtr<WebGLFramebuffer>
{
public:
    MOZ_DECLARE_REFCOUNTED_TYPENAME(WebGLFramebuffer)

    explicit WebGLFramebuffer(WebGLContext* context);

    struct Attachment
    {
        
        WebGLRefPtr<WebGLTexture> mTexturePtr;
        WebGLRefPtr<WebGLRenderbuffer> mRenderbufferPtr;
        GLenum mAttachmentPoint;
        TexImageTarget mTexImageTarget;
        GLint mTexImageLevel;
        mutable bool mNeedsFinalize;

        explicit Attachment(GLenum aAttachmentPoint = LOCAL_GL_COLOR_ATTACHMENT0);
        ~Attachment();

        bool IsDefined() const;

        bool IsDeleteRequested() const;

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

        void FinalizeAttachment(gl::GLContext* gl, GLenum attachmentLoc) const;
    };

    void Delete();

    void FramebufferRenderbuffer(GLenum target,
                                 GLenum attachment,
                                 GLenum rbtarget,
                                 WebGLRenderbuffer* wrb);

    void FramebufferTexture2D(GLenum target,
                              GLenum attachment,
                              TexImageTarget texImageTarget,
                              WebGLTexture* wtex,
                              GLint level);

private:
    void DetachAttachment(WebGLFramebuffer::Attachment& attachment);
    void DetachAllAttachments();
    const WebGLRectangleObject& GetAnyRectObject() const;
    Attachment* GetAttachmentOrNull(GLenum attachment);

public:
    bool HasDefinedAttachments() const;
    bool HasIncompleteAttachments() const;
    bool AllImageRectsMatch() const;
    GLenum PrecheckFramebufferStatus() const;
    GLenum CheckFramebufferStatus() const;
    GLenum GetFormatForAttachment(const WebGLFramebuffer::Attachment& attachment) const;

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

    const Attachment& GetAttachment(GLenum attachment) const;

    void DetachTexture(const WebGLTexture* tex);

    void DetachRenderbuffer(const WebGLRenderbuffer* rb);

    const WebGLRectangleObject& RectangleObject() const;

    WebGLContext* GetParentObject() const {
        return Context();
    }

    void FinalizeAttachments() const;

    virtual JSObject* WrapObject(JSContext* cx) MOZ_OVERRIDE;

    NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(WebGLFramebuffer)
    NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(WebGLFramebuffer)

    
    bool HasCompletePlanes(GLbitfield mask);

    bool CheckAndInitializeAttachments();

    bool CheckColorAttachmentNumber(GLenum attachment, const char* functionName) const;

    void EnsureColorAttachments(size_t colorAttachmentId);

    Attachment* AttachmentFor(GLenum attachment);
    void NotifyAttachableChanged() const;

private:
    ~WebGLFramebuffer() {
        DeleteOnce();
    }

    mutable GLenum mStatus;

    
    
    nsTArray<Attachment> mColorAttachments;
    Attachment mDepthAttachment,
               mStencilAttachment,
               mDepthStencilAttachment;
};

} 

#endif
