




#ifndef WEBGLFRAMEBUFFER_H_
#define WEBGLFRAMEBUFFER_H_

#include "WebGLObjectModel.h"

#include "nsWrapperCache.h"

#include "mozilla/LinkedList.h"

namespace mozilla {

class WebGLTexture;
class WebGLRenderbuffer;

class WebGLFramebuffer MOZ_FINAL
    : public nsISupports
    , public WebGLRefCountedObject<WebGLFramebuffer>
    , public LinkedListElement<WebGLFramebuffer>
    , public WebGLContextBoundObject
    , public nsWrapperCache
{
public:
    WebGLFramebuffer(WebGLContext *context);

    ~WebGLFramebuffer() {
        DeleteOnce();
    }

    class Attachment
    {
        
        WebGLRefPtr<WebGLTexture> mTexturePtr;
        WebGLRefPtr<WebGLRenderbuffer> mRenderbufferPtr;
        WebGLenum mAttachmentPoint;
        WebGLint mTextureLevel;
        WebGLenum mTextureCubeMapFace;

        friend class WebGLFramebuffer;

    public:
        Attachment(WebGLenum aAttachmentPoint)
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

    bool HasIncompleteAttachment() const {
        return (mColorAttachment.IsDefined() && !mColorAttachment.IsComplete()) ||
               (mDepthAttachment.IsDefined() && !mDepthAttachment.IsComplete()) ||
               (mStencilAttachment.IsDefined() && !mStencilAttachment.IsComplete()) ||
               (mDepthStencilAttachment.IsDefined() && !mDepthStencilAttachment.IsComplete());
    }

    bool HasDepthStencilConflict() const {
        return int(mDepthAttachment.IsDefined()) +
               int(mStencilAttachment.IsDefined()) +
               int(mDepthStencilAttachment.IsDefined()) >= 2;
    }

    bool HasAttachmentsOfMismatchedDimensions() const {
        return (mDepthAttachment.IsDefined() && !mDepthAttachment.HasSameDimensionsAs(mColorAttachment)) ||
               (mStencilAttachment.IsDefined() && !mStencilAttachment.HasSameDimensionsAs(mColorAttachment)) ||
               (mDepthStencilAttachment.IsDefined() && !mDepthStencilAttachment.HasSameDimensionsAs(mColorAttachment));
    }

    const Attachment& ColorAttachment() const {
        return mColorAttachment;
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
        return mColorAttachment.RectangleObject();
    }

    WebGLContext *GetParentObject() const {
        return Context();
    }

    virtual JSObject* WrapObject(JSContext *cx, JSObject *scope) MOZ_OVERRIDE;

    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(WebGLFramebuffer)

    bool CheckAndInitializeRenderbuffers();

    WebGLuint mGLName;
    bool mHasEverBeenBound;

    
    
    Attachment mColorAttachment,
               mDepthAttachment,
               mStencilAttachment,
               mDepthStencilAttachment;
};

} 

#endif
