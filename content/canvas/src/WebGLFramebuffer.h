




#ifndef WEBGLFRAMEBUFFER_H_
#define WEBGLFRAMEBUFFER_H_

#include "WebGLObjectModel.h"

#include "nsWrapperCache.h"

#include "mozilla/LinkedList.h"

namespace mozilla {

class WebGLTexture;
class WebGLRenderbuffer;

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
        WebGLenum mAttachmentPoint;
        WebGLint mTextureLevel;
        WebGLenum mTextureCubeMapFace;

        Attachment(WebGLenum aAttachmentPoint = LOCAL_GL_COLOR_ATTACHMENT0)
            : mAttachmentPoint(aAttachmentPoint)
        {}

        bool IsDefined() const {
            return Texture() || Renderbuffer();
        }

        bool IsDeleteRequested() const;

        bool HasAlpha() const;

        void SetTexture(WebGLTexture *tex, WebGLint level, WebGLenum face);
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
        WebGLint TextureLevel() const {
            return mTextureLevel;
        }
        WebGLenum TextureCubeMapFace() const {
            return mTextureCubeMapFace;
        }

        bool HasUninitializedRenderbuffer() const;

        void Reset() {
            mTexturePtr = nullptr;
            mRenderbufferPtr = nullptr;
        }

        const WebGLRectangleObject* RectangleObject() const;
        bool HasSameDimensionsAs(const Attachment& other) const;

        bool IsComplete() const;
    };

    void Delete();

    bool HasEverBeenBound() { return mHasEverBeenBound; }
    void SetHasEverBeenBound(bool x) { mHasEverBeenBound = x; }
    WebGLuint GLName() { return mGLName; }

    void FramebufferRenderbuffer(WebGLenum target,
                                 WebGLenum attachment,
                                 WebGLenum rbtarget,
                                 WebGLRenderbuffer *wrb);

    void FramebufferTexture2D(WebGLenum target,
                              WebGLenum attachment,
                              WebGLenum textarget,
                              WebGLTexture *wtex,
                              WebGLint level);

    bool HasIncompleteAttachment() const;

    bool HasDepthStencilConflict() const {
        return int(mDepthAttachment.IsDefined()) +
               int(mStencilAttachment.IsDefined()) +
               int(mDepthStencilAttachment.IsDefined()) >= 2;
    }

    bool HasAttachmentsOfMismatchedDimensions() const;

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

    const Attachment& GetAttachment(WebGLenum attachment) const;

    void DetachTexture(const WebGLTexture *tex);

    void DetachRenderbuffer(const WebGLRenderbuffer *rb);

    const WebGLRectangleObject *RectangleObject() {
        return mColorAttachments[0].RectangleObject();
    }

    WebGLContext *GetParentObject() const {
        return Context();
    }

    virtual JSObject* WrapObject(JSContext *cx,
                                 JS::Handle<JSObject*> scope) MOZ_OVERRIDE;

    NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(WebGLFramebuffer)
    NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(WebGLFramebuffer)

    bool CheckAndInitializeRenderbuffers();

    bool CheckColorAttachementNumber(WebGLenum attachment, const char * functionName) const;

    WebGLuint mGLName;
    bool mHasEverBeenBound;

    void EnsureColorAttachments(size_t colorAttachmentId);

    
    
    nsTArray<Attachment> mColorAttachments;
    Attachment mDepthAttachment,
               mStencilAttachment,
               mDepthStencilAttachment;
};

} 

#endif
