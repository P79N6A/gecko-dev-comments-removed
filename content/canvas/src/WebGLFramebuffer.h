




#ifndef WEBGLFRAMEBUFFER_H_
#define WEBGLFRAMEBUFFER_H_

#include "WebGLObjectModel.h"

#include "nsWrapperCache.h"

#include "mozilla/LinkedList.h"
#include "mozilla/TypedEnum.h"

namespace mozilla {

class WebGLFramebufferAttachable;
class WebGLTexture;
class WebGLRenderbuffer;
namespace gl {
    class GLContext;
}

class WebGLFramebuffer MOZ_FINAL
    : public nsWrapperCache
    , public WebGLRefCountedObject<WebGLFramebuffer>
    , public LinkedListElement<WebGLFramebuffer>
    , public WebGLContextBoundObject
{
public:
    WebGLFramebuffer(WebGLContext* context);

    struct Attachment
    {
        
        WebGLRefPtr<WebGLTexture> mTexturePtr;
        WebGLRefPtr<WebGLRenderbuffer> mRenderbufferPtr;
        GLenum mAttachmentPoint;
        GLenum mTexImageTarget;
        GLint mTexImageLevel;
        mutable bool mNeedsFinalize;

        Attachment(GLenum aAttachmentPoint = LOCAL_GL_COLOR_ATTACHMENT0)
            : mAttachmentPoint(aAttachmentPoint)
            , mNeedsFinalize(false)
        {}

        bool IsDefined() const {
            return Texture() || Renderbuffer();
        }

        bool IsDeleteRequested() const;

        bool HasAlpha() const;

        
        MOZ_BEGIN_NESTED_ENUM_CLASS(FloatType)
            Any = 0,
            Half,
            Full
        MOZ_END_NESTED_ENUM_CLASS(FloatType)

        bool IsFloatType(FloatType floatType = FloatType::Any) const;

        void SetTexImage(WebGLTexture* tex, GLenum target, GLint level);
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
        GLenum TexImageTarget() const {
            return mTexImageTarget;
        }
        GLint TexImageLevel() const {
            return mTexImageLevel;
        }

        bool HasUninitializedImageData() const;
        void SetImageDataStatus(WebGLImageDataStatus x);

        void Reset() {
            mTexturePtr = nullptr;
            mRenderbufferPtr = nullptr;
        }

        const WebGLRectangleObject& RectangleObject() const;

        bool HasImage() const;
        bool IsComplete() const;

        void FinalizeAttachment(gl::GLContext* gl, GLenum attachmentLoc) const;
    };

    void Delete();

    bool HasEverBeenBound() { return mHasEverBeenBound; }
    void SetHasEverBeenBound(bool x) { mHasEverBeenBound = x; }
    GLuint GLName() { return mGLName; }

    void FramebufferRenderbuffer(GLenum target,
                                 GLenum attachment,
                                 GLenum rbtarget,
                                 WebGLRenderbuffer* wrb);

    void FramebufferTexture2D(GLenum target,
                              GLenum attachment,
                              GLenum textarget,
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

    GLuint mGLName;
    bool mHasEverBeenBound;

    
    
    nsTArray<Attachment> mColorAttachments;
    Attachment mDepthAttachment,
               mStencilAttachment,
               mDepthStencilAttachment;
};

MOZ_FINISH_NESTED_ENUM_CLASS(WebGLFramebuffer::Attachment::FloatType)

} 

#endif
