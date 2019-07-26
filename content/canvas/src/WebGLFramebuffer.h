




#ifndef WEBGLFRAMEBUFFER_H_
#define WEBGLFRAMEBUFFER_H_

#include "WebGLObjectModel.h"

#include "nsWrapperCache.h"

#include "mozilla/LinkedList.h"

namespace mozilla {

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
    WebGLFramebuffer(WebGLContext *context);

    ~WebGLFramebuffer() {
        DeleteOnce();
    }

    struct Attachment
    {
        
        WebGLRefPtr<WebGLTexture> mTexturePtr;
        WebGLRefPtr<WebGLRenderbuffer> mRenderbufferPtr;
        GLenum mAttachmentPoint;
        GLenum mTexImageTarget;
        GLint mTexImageLevel;

        Attachment(GLenum aAttachmentPoint = LOCAL_GL_COLOR_ATTACHMENT0)
            : mAttachmentPoint(aAttachmentPoint)
        {}

        bool IsDefined() const {
            return Texture() || Renderbuffer();
        }

        bool IsDeleteRequested() const;

        bool HasAlpha() const;

        void SetTexImage(WebGLTexture *tex, GLenum target, GLint level);
        void SetRenderbuffer(WebGLRenderbuffer *rb) {
            mTexturePtr = nullptr;
            mRenderbufferPtr = rb;
        }
        const WebGLTexture *Texture() const {
            return mTexturePtr;
        }
        WebGLTexture *Texture() {
            return mTexturePtr;
        }
        const WebGLRenderbuffer *Renderbuffer() const {
            return mRenderbufferPtr;
        }
        WebGLRenderbuffer *Renderbuffer() {
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

        const WebGLRectangleObject* RectangleObject() const;
        bool HasSameDimensionsAs(const Attachment& other) const;

        bool IsComplete() const;

        void FinalizeAttachment(GLenum attachmentLoc) const;
    };

    void Delete();

    bool HasEverBeenBound() { return mHasEverBeenBound; }
    void SetHasEverBeenBound(bool x) { mHasEverBeenBound = x; }
    GLuint GLName() { return mGLName; }

    void FramebufferRenderbuffer(GLenum target,
                                 GLenum attachment,
                                 GLenum rbtarget,
                                 WebGLRenderbuffer *wrb);

    void FramebufferTexture2D(GLenum target,
                              GLenum attachment,
                              GLenum textarget,
                              WebGLTexture *wtex,
                              GLint level);

    bool HasIncompleteAttachment() const;

    bool HasDepthStencilConflict() const {
        return int(mDepthAttachment.IsDefined()) +
               int(mStencilAttachment.IsDefined()) +
               int(mDepthStencilAttachment.IsDefined()) >= 2;
    }

    bool HasAttachmentsOfMismatchedDimensions() const;

    const size_t ColorAttachmentCount() const {
        return mColorAttachments.Length();
    }
    const Attachment& ColorAttachment(uint32_t colorAttachmentId) const {
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

    void DetachTexture(const WebGLTexture *tex);

    void DetachRenderbuffer(const WebGLRenderbuffer *rb);

    const WebGLRectangleObject *RectangleObject() {
        return mColorAttachments[0].RectangleObject();
    }

    WebGLContext *GetParentObject() const {
        return Context();
    }

    void FinalizeAttachments() const;

    virtual JSObject* WrapObject(JSContext *cx,
                                 JS::Handle<JSObject*> scope) MOZ_OVERRIDE;

    NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(WebGLFramebuffer)
    NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(WebGLFramebuffer)

    bool CheckAndInitializeAttachments();

    bool CheckColorAttachementNumber(GLenum attachment, const char * functionName) const;

    GLuint mGLName;
    bool mHasEverBeenBound;

    void EnsureColorAttachments(size_t colorAttachmentId);

    
    
    nsTArray<Attachment> mColorAttachments;
    Attachment mDepthAttachment,
               mStencilAttachment,
               mDepthStencilAttachment;
};

} 

#endif
